#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#if defined(__ANDROID__)
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace rex::graphics::gta4_native {

// Bounded background compiler with 1-2 Little-core worker threads. Callers
// publish results on their owner thread. Stop joins the active driver call;
// shader modules, layouts, caches and the device must outlive this object.
template <typename Key, typename Result, typename Hash>
class NativePipelineCompiler {
 public:
  using Compile = std::function<Result()>;
  explicit NativePipelineCompiler(size_t capacity, std::function<void()> idle = {},
                                  size_t worker_count = 1, const char* thread_name = nullptr)
      : capacity_(std::max<size_t>(capacity, 1)),
        idle_(std::move(idle)),
        thread_name_(thread_name ? thread_name : "") {
    const size_t workers = std::clamp<size_t>(worker_count, 1, 2);
    threads_.reserve(workers);
    for (size_t i = 0; i < workers; ++i) {
      threads_.emplace_back([this] { Run(); });
    }
  }
  ~NativePipelineCompiler() { Stop(); }
  NativePipelineCompiler(const NativePipelineCompiler&) = delete;
  NativePipelineCompiler& operator=(const NativePipelineCompiler&) = delete;

  bool Enqueue(const Key& key, Compile compile, bool demanded = false) {
    std::lock_guard lock(mutex_);
    if (stopping_) {
      return false;
    }
    if (jobs_.contains(key)) {
      return true;
    }
    if (jobs_.size() >= capacity_) {
      if (!demanded || queue_.empty()) {
        return false;
      }
      // Drop only an unstarted speculative recipe to admit an exact first use.
      jobs_.erase(queue_.back());
      queue_.pop_back();
    }
    auto job = std::make_shared<Job>();
    job->compile = std::move(compile);
    jobs_.emplace(key, job);
    if (demanded) {
      queue_.push_front(key);
    } else {
      queue_.push_back(key);
    }
    changed_.notify_all();
    return true;
  }

  // Demand promotes an existing speculative job, preserving exact deduplication.
  // No renderer state or Vulkan lifetime is changed while waiting.
  std::optional<Result> Take(const Key& key, bool wait) {
    std::unique_lock lock(mutex_);
    const auto found = jobs_.find(key);
    if (found == jobs_.end()) {
      return std::nullopt;
    }
    const auto job = found->second;
    if (wait && !job->result) {
      const auto queued = std::find(queue_.begin(), queue_.end(), key);
      if (queued != queue_.end() && queued != queue_.begin()) {
        queue_.erase(queued);
        queue_.push_front(key);
      }
      changed_.notify_all();
      changed_.wait(lock, [&] { return job->result.has_value() || stopping_; });
    }
    if (!job->result) {
      return std::nullopt;
    }
    Result result = std::move(*job->result);
    jobs_.erase(key);
    return result;
  }

  std::vector<std::pair<Key, Result>> TakeCompleted() {
    std::lock_guard lock(mutex_);
    std::vector<std::pair<Key, Result>> completed;
    for (auto it = jobs_.begin(); it != jobs_.end();) {
      if (it->second->result) {
        completed.emplace_back(it->first, std::move(*it->second->result));
        it = jobs_.erase(it);
      } else {
        ++it;
      }
    }
    return completed;
  }

  bool Contains(const Key& key) const {
    std::lock_guard lock(mutex_);
    return jobs_.contains(key);
  }
  size_t PendingCount() const {
    std::lock_guard lock(mutex_);
    return queue_.size() + active_workers_;
  }
  void WaitUntilIdle() {
    std::unique_lock lock(mutex_);
    changed_.wait(lock, [this] { return active_workers_ == 0 && queue_.empty(); });
  }
  bool WaitUntilIdleFor(std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout,
                             [this] { return active_workers_ == 0 && queue_.empty(); });
  }
  void Stop() {
    {
      std::lock_guard lock(mutex_);
      stopping_ = true;
      queue_.clear();
      changed_.notify_all();
    }
    for (auto& thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

 private:
  struct Job {
    Compile compile;
    std::optional<Result> result;
  };
  static void ConfigureWorkerThread(const std::string& thread_name) {
#if defined(__ANDROID__)
    if (!thread_name.empty()) {
      pthread_setname_np(pthread_self(), thread_name.c_str());
    }
    setpriority(PRIO_PROCESS, 0, 10);
    const long ncpus = sysconf(_SC_NPROCESSORS_CONF);
    const int little_count = (ncpus > 0 && ncpus <= 16) ? int(ncpus / 2) : 4;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (int cpu = 0; cpu < std::max(little_count, 1); ++cpu) {
      CPU_SET(cpu, &mask);
    }
    sched_setaffinity(0, sizeof(mask), &mask);
#else
    (void)thread_name;
#endif
  }
  void Run() {
    ConfigureWorkerThread(thread_name_);
    std::unique_lock lock(mutex_);
    for (;;) {
      changed_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
      if (stopping_) {
        return;
      }
      const Key key = queue_.front();
      queue_.pop_front();
      const auto job = jobs_.at(key);
      ++active_workers_;
      lock.unlock();
      Result result{};
      try {
        result = job->compile();
      } catch (...) {
        // Allocation/cache I/O failure is an unsuccessful job, never process
        // termination on a background thread. The owner retains exact fallback.
      }
      lock.lock();
      job->result = std::move(result);
      job->compile = {};
      --active_workers_;
      changed_.notify_all();
      if (!stopping_ && active_workers_ == 0 && queue_.empty() && idle_) {
        ++active_workers_;
        lock.unlock();
        try {
          idle_();
        } catch (...) {
        }
        lock.lock();
        --active_workers_;
        changed_.notify_all();
      }
    }
  }

  const size_t capacity_;
  std::function<void()> idle_;
  std::string thread_name_;
  mutable std::mutex mutex_;
  std::condition_variable changed_;
  bool stopping_ = false;
  size_t active_workers_ = 0;
  std::deque<Key> queue_;
  std::unordered_map<Key, std::shared_ptr<Job>, Hash> jobs_;
  std::vector<std::thread> threads_;
};

}  // namespace rex::graphics::gta4_native
