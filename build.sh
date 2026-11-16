#!/bin/bash
# Build Maket and open it.
#   ./build.sh          build (Release) and launch
#   ./build.sh debug    build a Debug version and launch
#   ./build.sh xcode    generate an Xcode project in build-xcode/ and open it
# (./build.sh tests, the console test program, arrives with stage 0b.)
set -e
cd "$(dirname "$0")"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

if [ "$1" = "xcode" ]; then
    cmake -B build-xcode -G Xcode
    open build-xcode/Maket.xcodeproj
    exit 0
fi

if [ "$1" = "tests" ]; then
    echo "The console test program arrives with stage 0b."
    exit 1
fi

CONFIG=Release
[ "$1" = "debug" ] && CONFIG=Debug

cmake -B build -DCMAKE_BUILD_TYPE=$CONFIG
cmake --build build --config $CONFIG -j "$JOBS"

APP="build/Maket_artefacts/$CONFIG/Maket.app"
if [ -d "$APP" ]; then
    echo "Built: $APP"
    open "$APP"
fi
