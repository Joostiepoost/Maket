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
set(MAKET_BUILD_STAGE "0a")

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

set(MAKET_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    MAKET_BUILD_STAGE="${MAKET_BUILD_STAGE}")

# --- The app -----------------------------------------------------------------
# From stage 0b on, the sources without interface code (model, file formats,
# engines, notation) are listed apart, so that the console test program can
# build them too.
juce_add_gui_app(Maket
    PRODUCT_NAME        "Maket"
    COMPANY_NAME        "Maket"
    BUNDLE_ID           "com.maket.app"
    VERSION             ${PROJECT_VERSION}
    MICROPHONE_PERMISSION_ENABLED FALSE)

juce_generate_juce_header(Maket)

target_sources(Maket PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/UI/Theme.cpp
    Source/UI/Tabs.cpp
    Source/UI/TabBar.cpp
    Source/UI/EmptyPage.cpp
    Source/UI/Inspector.cpp)

target_compile_definitions(Maket PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Maket,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Maket,JUCE_VERSION>"
    ${MAKET_DEFINITIONS})

target_link_libraries(Maket
    PRIVATE
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
