cmake_minimum_required(VERSION 3.26)
project(AVA_C LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------
# Options
# -------------------------
option(AVA_ENABLE_FFT "Enable KissFFT analyzer if available" OFF)

# -------------------------
# Dependencies via vcpkg
# -------------------------
find_package(SDL2 CONFIG REQUIRED)
find_package(RtAudio CONFIG REQUIRED)
# OpenGL on Windows (opengl32) — no find_package needed

# Optional KissFFT (quiet, only if available)
if(AVA_ENABLE_FFT)
    find_package(kissfft CONFIG QUIET)
endif()

# -------------------------
# DaisySP (build from source in subdir)
# -------------------------
add_subdirectory(DaisySP)

# -------------------------
# Subdirectories
# -------------------------
add_subdirectory(core)    # <-- builds ava_core
add_subdirectory(ui)
add_subdirectory(dsp)
add_subdirectory(audio)
add_subdirectory(app)

# -------------------------
# Global link libraries
# -------------------------
set(AVA_LINK_LIBS
    SDL2::SDL2
    RtAudio::rtaudio
    DaisySP
)

set(AVA_LINK_LIBS ${AVA_LINK_LIBS} PARENT_SCOPE)
