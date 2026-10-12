cmake_minimum_required(VERSION 3.22)
project(PAS1 VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PAS1_BUILD_PLUGIN "Build the VST3 and standalone instrument" ON)
option(PAS1_BUILD_TESTS "Build the dependency-free DSP tests" ON)

if(PAS1_BUILD_TESTS)
    add_executable(PAS1Tests
        Tests/CoreTests.cpp
        Source/DSP/Oscillator.cpp
        Source/DSP/AmpEnvelope.cpp
        Source/DSP/LowFrequencyOscillator.cpp
        Source/DSP/StateVariableFilter.cpp
        Source/DSP/WavetableOscillator.cpp
        Source/DSP/EffectsChain.cpp
        Source/DSP/MorphMatrix.cpp
        Source/DSP/ModulationMatrix.cpp
        Source/DSP/Portamento.cpp
        Source/DSP/SoundDNA.cpp
        Source/Generation/SoundGenerator.cpp
    )
    target_include_directories(PAS1Tests PRIVATE Source)
    if(MSVC)
        target_compile_options(PAS1Tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(PAS1Tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME PAS1CoreTests COMMAND PAS1Tests)
endif()

if(PAS1_BUILD_PLUGIN)
    # JUCE 8.0.8 is pinned for repeatable local and CI builds.
    FetchContent_Declare(
        JUCE
        URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.8.tar.gz
        URL_HASH SHA256=08abd711eb0345972974d589648f5c8829cb478bc513ddb34836664b0b36d152
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(PAS1
        COMPANY_NAME "PAS Audio"
        BUNDLE_ID "audio.pas.pas1"
        PLUGIN_MANUFACTURER_CODE Pasa
        PLUGIN_CODE Pas1
        PRODUCT_NAME "PAS 1"
        DESCRIPTION "Adaptive hybrid synthesizer"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        FORMATS VST3 Standalone
    )

    juce_generate_juce_header(PAS1)
    target_sources(PAS1 PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/UI/ParameterPage.cpp
        Source/UI/SpectrumAnalyzer.cpp
        Source/DSP/Oscillator.cpp
        Source/DSP/AmpEnvelope.cpp
        Source/DSP/LowFrequencyOscillator.cpp
        Source/DSP/StateVariableFilter.cpp
        Source/DSP/WavetableOscillator.cpp
        Source/DSP/EffectsChain.cpp
        Source/DSP/MorphMatrix.cpp
        Source/DSP/ModulationMatrix.cpp
        Source/DSP/Portamento.cpp
        Source/DSP/SoundDNA.cpp
        Source/Presets/PresetManager.cpp
        Source/Presets/PresetsPanel.cpp
        Source/Generation/SoundGenerator.cpp
        Source/Generation/GeneratorPanel.cpp
        Source/Generation/MorphPanel.cpp
        Source/Generation/ModulationPanel.cpp
        Source/Generation/SoundDNAPanel.cpp
    )
    target_include_directories(PAS1 PRIVATE Source)
    target_compile_definitions(PAS1 PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
    )
    target_link_libraries(PAS1 PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
endif()
