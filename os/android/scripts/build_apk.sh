#!/bin/bash
# Package the staged native libraries and resources into an APK.
# usage: build_apk.sh [debug|release]   (default debug)
set -euo pipefail
. "$(dirname "$0")/env.sh"
VARIANT="${1:-debug}"
TASK="assemble$(tr '[:lower:]' '[:upper:]' <<< "${VARIANT:0:1}")${VARIANT:1}"

[ -f "$JNILIBS/libmain.so" ] || { echo "no libmain.so staged - run build_native.sh"; exit 1; }

cat > "$APP_DIR/local.properties" <<EOF
sdk.dir=$(cygpath -m "$ANDROID_HOME" 2>/dev/null || echo "$ANDROID_HOME")
EOF

export JAVA_HOME="C:\Users\Ad\.jdks\jbr-21.0.11" 
export GRADLE_USER_HOME="G:/cod/cacheSlopodroid/.gradle"
export ANDROID_HOME="G:/cod/cacheSlopodroid"
export ANDROID_SDK_ROOT="G:/cod/cacheSlopodroid"
export ANDROID_USER_HOME="G:/cod/cacheSlopodroid/.android"

cd "$APP_DIR"
./gradlew --no-daemon --gradle-user-home "G:/cod/cacheSlopodroid/.gradle" "$TASK" --stacktrace
ls -la "$APP_DIR/app/build/outputs/apk/$VARIANT/"
