# OHOS native DSP module for @quizzyjoy/audio-dsp
cmake_minimum_required(VERSION 3.5.0)
project(audio_dsp)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH})
include_directories(${NATIVERENDER_ROOT_PATH}/third_party/kissfft)

add_library(audio_dsp SHARED
    napi_init.cpp
    audio_dsp.cpp
    third_party/kissfft/kiss_fft.c
    third_party/kissfft/kiss_fftr.c)

target_compile_definitions(audio_dsp PRIVATE kiss_fft_scalar=float)

target_link_libraries(audio_dsp PUBLIC
    libace_napi.z.so
    libhilog_ndk.z.so
    m)
