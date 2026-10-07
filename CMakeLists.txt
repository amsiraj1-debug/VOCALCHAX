cmake_minimum_required(VERSION 3.22)

project(VOCALCHAX VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

option(VOCALCHAX_ENABLE_ONNX "Enable optional ONNX Runtime voice model backend" OFF)
set(VOCALCHAX_ONNXRUNTIME_ROOT "" CACHE PATH "Path to an extracted ONNX Runtime package")

juce_add_plugin(VOCALCHAX
    COMPANY_NAME "VOCALCHAX"
    BUNDLE_ID "com.vocalchax.plugin"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE VChx
    PLUGIN_CODE Vc01
    FORMATS VST3
    PRODUCT_NAME "VOCALCHAX"
    VST3_CATEGORIES Fx
)

target_sources(VOCALCHAX PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Dsp/VocalShaper.cpp
    Source/Dsp/VocalShaper.h
    Source/OnnxVoiceEngine.cpp
    Source/OnnxVoiceEngine.h
)

target_compile_definitions(VOCALCHAX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VOCALCHAX PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(VOCALCHAX_ENABLE_ONNX)
    if(NOT VOCALCHAX_ONNXRUNTIME_ROOT)
        message(FATAL_ERROR "Set VOCALCHAX_ONNXRUNTIME_ROOT to an extracted ONNX Runtime package.")
    endif()

    find_path(ONNXRUNTIME_INCLUDE_DIR
        NAMES onnxruntime_cxx_api.h
        PATHS "${VOCALCHAX_ONNXRUNTIME_ROOT}/include"
        NO_DEFAULT_PATH
        REQUIRED
    )

    find_library(ONNXRUNTIME_LIBRARY
        NAMES onnxruntime
        PATHS "${VOCALCHAX_ONNXRUNTIME_ROOT}/lib"
        NO_DEFAULT_PATH
        REQUIRED
    )

    target_include_directories(VOCALCHAX PRIVATE "${ONNXRUNTIME_INCLUDE_DIR}")
    target_link_libraries(VOCALCHAX PRIVATE "${ONNXRUNTIME_LIBRARY}")
    target_compile_definitions(VOCALCHAX PRIVATE VOCALCHAX_ENABLE_ONNX=1)
else()
    target_compile_definitions(VOCALCHAX PRIVATE VOCALCHAX_ENABLE_ONNX=0)
endif()
