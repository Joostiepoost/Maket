#!/bin/bash
# Build Maket and open it.
#   ./build.sh          build (Release) and launch
#   ./build.sh debug    build a Debug version and launch
#   ./build.sh xcode    generate an Xcode project in build-xcode/ and open it
#   ./build.sh tests    build and run the console test program (report in Measurements/)
set -e
cd "$(dirname "$0")"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

if [ "$1" = "xcode" ]; then
    cmake -B build-xcode -G Xcode
    open build-xcode/Maket.xcodeproj
    exit 0
fi

if [ "$1" = "tests" ]; then
    shift
    # Use the JUCE the app build already has, so that nothing is downloaded twice.
    JUCE_ARG=()
    if [ -d build/_deps/juce-src ]; then
        JUCE_ARG=("-DMAKET_JUCE_PATH=$(pwd)/build/_deps/juce-src")
    elif [ -f build/CMakeCache.txt ]; then
        JUCE_PATH="$(sed -n 's/^MAKET_JUCE_PATH:[A-Z]*=//p' build/CMakeCache.txt)"
        if [ -n "$JUCE_PATH" ]; then JUCE_ARG=("-DMAKET_JUCE_PATH=$JUCE_PATH"); fi
    fi
    cmake -B build-tests -DCMAKE_BUILD_TYPE=Release -DMAKET_BUILD_TESTS=ON "${JUCE_ARG[@]}"
    cmake --build build-tests --config Release --target MaketTests -j "$JOBS"
    STATUS=0
    ./build-tests/MaketTests_artefacts/Release/MaketTests --out Measurements "$@" || STATUS=$?
    echo "Report: Measurements/report.md"
    exit $STATUS
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
