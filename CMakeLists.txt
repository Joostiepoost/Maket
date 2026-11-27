# Maket — composing with snippets: a roll with slots, a build surface, a render
# Build instructions: see README.md (short version: ./build.sh)

cmake_minimum_required(VERSION 3.22)

# Apple Silicon only, macOS 12 or newer.
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "")

project(Maket VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# The build stage this source stands at; shown at the right of the tab row,
# so that it is clear which build is being tested.
set(MAKET_BUILD_STAGE "3b.1")

# --- JUCE --------------------------------------------------------------------
# By default JUCE is downloaded automatically (first build takes a few minutes).
# Set MAKET_JUCE_PATH to use a local copy instead (for example Projet's or
# Trajet's build/_deps/juce-src, to skip the download).
set(MAKET_JUCE_PATH "" CACHE PATH "Optional path to a local JUCE checkout")

if(MAKET_JUCE_PATH)
    add_subdirectory(${MAKET_JUCE_PATH} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        9.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# --- Sources without interface code --------------------------------------------
# Model, file formats, engines and notation. They are listed apart because the
# console test program builds them too, without the window. Nothing in this
# list may include interface code (Source/UI, MainComponent).
set(MAKET_CORE_SOURCES
    Source/Model/AppInfo.cpp
    Source/Model/Settings.cpp
    Source/Model/WindowState.cpp
    Source/Model/Project.cpp
    Source/Notation/Miniature.cpp
    Source/Model/Llll.cpp
    Source/Model/Roll.cpp
    Source/Formats/LlllText.cpp
    Source/Formats/BachRoll.cpp
    Source/Engine/Osc.cpp
    Source/Engine/OscSender.cpp
    Source/Engine/OscScheduler.cpp
    Source/Engine/ConTimbre.cpp
    Source/Engine/TimingTest.cpp
    Source/Engine/ConTimbreMapping.cpp
    Source/Engine/PlayerStudy.cpp
    Source/Engine/Playing.cpp
    Source/Engine/StreamClock.cpp
    Source/Engine/Synth.cpp
    Source/Engine/Transport.cpp
    Source/Notation/Chords.cpp
    Source/Notation/Glides.cpp
    Source/Notation/Spelling.cpp
    Source/Notation/Staves.cpp)

set(MAKET_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    MAKET_VERSION="${PROJECT_VERSION}"
    MAKET_BUILD_STAGE="${MAKET_BUILD_STAGE}")

# --- The app -----------------------------------------------------------------
# NSAppSleepDisabled: macOS does not "nap" Maket when it is not in front. A
# napping app's timers fire up to a tenth of a second late, and so would its
# notes (measured at stage 1d). Source/Platform/BusyActivity says the same to
# the system while Maket plays; this entry holds whatever the app is doing.
juce_add_gui_app(Maket
    PRODUCT_NAME        "Maket"
    COMPANY_NAME        "Maket"
    BUNDLE_ID           "com.maket.app"
    VERSION             ${PROJECT_VERSION}
    MICROPHONE_PERMISSION_ENABLED FALSE
    PLIST_TO_MERGE      "<plist><dict><key>NSAppSleepDisabled</key><true/><key>LSAppNapIsDisabled</key><true/></dict></plist>")

juce_generate_juce_header(Maket)

target_sources(Maket PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/UI/Theme.cpp
    Source/UI/Tabs.cpp
    Source/UI/TabBar.cpp
    Source/UI/EmptyPage.cpp
    Source/UI/Inspector.cpp
    Source/UI/PanelButton.cpp
    Source/UI/PreferencesPage.cpp
    Source/UI/UtilitiesPage.cpp
    Source/UI/MusicFont.cpp
    Source/UI/RollView.cpp
    Source/UI/EditionPage.cpp
    Source/UI/LibraryPage.cpp
    Source/Audio/AudioOutput.cpp
    Source/UI/OscTestPage.cpp
    Source/UI/TimingTestPage.cpp
    Source/UI/ConTimbreTestPage.cpp
    Source/UI/PlayerStudyPage.cpp
    ${MAKET_CORE_SOURCES})

# What differs per operating system. On macOS a few lines of Objective-C.
if(APPLE)
    target_sources(Maket PRIVATE Source/Platform/BusyActivity.mm)
else()
    target_sources(Maket PRIVATE Source/Platform/BusyActivity.cpp)
endif()

target_compile_definitions(Maket PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Maket,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Maket,JUCE_VERSION>"
    ${MAKET_DEFINITIONS})

# Files built into the app: the Bravura music font (SIL Open Font License, see Resources/Fonts).
juce_add_binary_data(MaketData SOURCES Resources/Fonts/Bravura.otf)

target_link_libraries(Maket
    PRIVATE
        MaketData
        juce::juce_audio_devices
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Test program (./build.sh tests) -----------------------------------------
# A console program that builds the sources without interface code, runs its
# checks on them and writes Measurements/report.md. Each build step adds its
# own section to it (Tests/MaketTests.cpp).
option(MAKET_BUILD_TESTS "Build the MaketTests console program" OFF)

if(MAKET_BUILD_TESTS)
    juce_add_console_app(MaketTests PRODUCT_NAME "MaketTests")
    juce_generate_juce_header(MaketTests)
    target_sources(MaketTests PRIVATE Tests/MaketTests.cpp Tests/BachTests.cpp Tests/BachWriteTests.cpp Tests/OscTests.cpp Tests/TimingTests.cpp Tests/MappingTests.cpp Tests/StudyTests.cpp Tests/NotationTests.cpp Tests/PlayingTests.cpp Tests/SynthTests.cpp Tests/SpeedTests.cpp Tests/StateTests.cpp Tests/ProjectTests.cpp ${MAKET_CORE_SOURCES})
    target_compile_definitions(MaketTests PRIVATE
        ${MAKET_DEFINITIONS} JUCE_STANDALONE_APPLICATION=1
        MAKET_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(MaketTests
        PRIVATE
            juce::juce_core
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()
