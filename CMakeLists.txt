cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Native Apple Silicon build")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(VibeSpark VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(VIBESPARK_LIVE_MODE "Instrument-type plugin with silent audio out for Ableton Live" ON)
option(VIBESPARK_COPY_AFTER_BUILD "Copy VST3 to ~/Library/Audio/Plug-Ins/VST3 after build" ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

if(VIBESPARK_LIVE_MODE)
    set(VS_IS_SYNTH TRUE)
    set(VS_IS_MIDI_FX FALSE)
    set(VS_VST3_CAT "Instrument Synth")
else()
    set(VS_IS_SYNTH FALSE)
    set(VS_VST3_CAT "Fx")
endif()

juce_add_plugin(VibeSpark
    COMPANY_NAME                "VibeLab"
    PLUGIN_MANUFACTURER_CODE    Vblb
    PLUGIN_CODE                 Vsp1
    FORMATS                     VST3
    PRODUCT_NAME                "VibeSpark"
    IS_SYNTH                    ${VS_IS_SYNTH}
    IS_MIDI_EFFECT              ${VS_IS_MIDI_FX}
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${VIBESPARK_COPY_AFTER_BUILD}
    VST3_CATEGORIES             ${VS_VST3_CAT})

target_sources(VibeSpark PRIVATE
    Source/ChordGen.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

target_compile_definitions(VibeSpark PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VibeSpark
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
