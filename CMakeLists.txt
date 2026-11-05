cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()
# Static MSVC runtime on Windows: users do not need the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Peaky VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

set(PEAKY_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PEAKY_FORMATS AU)
endif()

juce_add_plugin(Peaky
    COMPANY_NAME            "Aron H"
    COMPANY_WEBSITE         "https://www.instagram.com/aronh.official"
    PLUGIN_MANUFACTURER_CODE ArnH
    PLUGIN_CODE             Peky
    FORMATS                 ${PEAKY_FORMATS}
    PRODUCT_NAME            "Peaky"
    BUNDLE_ID               com.aronh.peaky
    DESCRIPTION             "Automatic pre-compression vocal peak smoother"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    VST3_CATEGORIES         Fx Dynamics
    AU_MAIN_TYPE            kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Peaky PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PresetManager.cpp)

target_compile_definitions(Peaky PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Peaky
    PRIVATE juce::juce_audio_utils
            juce::juce_dsp
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
