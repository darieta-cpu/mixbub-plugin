# Windows: runtime MSVC statico, cosi' non serve installare i Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Per Apple Silicon + Intel (deve stare PRIMA di project())
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

cmake_minimum_required(VERSION 3.22)
project(MixbusChain VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: usa la cartella locale "JUCE" se presente (o -DJUCE_PATH=...), altrimenti la scarica da GitHub.
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Percorso della cartella JUCE")
if(EXISTS "${JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("${JUCE_PATH}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Copia automatica in ~/Library/Audio/Plug-Ins (disattivata in CI con -DMC_COPY=FALSE)
set(MC_COPY TRUE CACHE BOOL "Copia il plugin dopo il build")

juce_add_plugin(MixbusChain
    COMPANY_NAME "DaniArieta"
    PLUGIN_MANUFACTURER_CODE Dari
    PLUGIN_CODE Mbch
    FORMATS VST3 AU            # aggiungi/togli formati qui
    PRODUCT_NAME "Mixbus Chain"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD ${MC_COPY}
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Dynamics EQ)

target_sources(MixbusChain PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/UI/Look.h
    Source/UI/Meters.h
    Source/UI/Controls.h
    Source/DSP/DspCommon.h
    Source/DSP/Tape.h
    Source/DSP/Filters.h
    Source/DSP/PultecEQ.h
    Source/DSP/BusCompressor.h
    Source/DSP/Limiter.h)

target_compile_definitions(MixbusChain PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixbusChain
    PRIVATE juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
