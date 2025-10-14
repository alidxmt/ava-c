cmake_minimum_required(VERSION 3.14)

project(DAISYSP VERSION 0.0.1)

add_library(DaisySP STATIC
  Source/Control/adenv.cpp
  Source/Control/adsr.cpp
  Source/Control/phasor.cpp
  Source/Drums/analogbassdrum.cpp
  Source/Drums/analogsnaredrum.cpp
  Source/Drums/hihat.cpp
  Source/Drums/synthbassdrum.cpp
  Source/Drums/synthsnaredrum.cpp
  Source/Dynamics/crossfade.cpp
  Source/Dynamics/limiter.cpp
  Source/Effects/autowah.cpp
  Source/Effects/chorus.cpp
  Source/Effects/decimator.cpp
  Source/Effects/flanger.cpp
  Source/Effects/overdrive.cpp
  Source/Effects/phaser.cpp
  Source/Effects/sampleratereducer.cpp
  Source/Effects/tremolo.cpp
  Source/Effects/reverbsc.cpp     # ✅ single reverbsc
  Source/Effects/wavefolder.cpp
  # Source/Filters/ladder.cpp     # ❌ disable for MSVC
  Source/Filters/svf.cpp
  Source/Filters/soap.cpp
  Source/Noise/clockednoise.cpp
  Source/Noise/grainlet.cpp
  Source/Noise/particle.cpp
  Source/PhysicalModeling/drip.cpp
  Source/PhysicalModeling/modalvoice.cpp
  Source/PhysicalModeling/resonator.cpp
  Source/PhysicalModeling/KarplusString.cpp
  Source/PhysicalModeling/stringvoice.cpp
  Source/Sampling/granularplayer.cpp
  Source/Synthesis/fm2.cpp
  Source/Synthesis/formantosc.cpp
  Source/Synthesis/oscillator.cpp
  Source/Synthesis/oscillatorbank.cpp
  Source/Synthesis/variablesawosc.cpp
  Source/Synthesis/variableshapeosc.cpp
  Source/Synthesis/vosim.cpp
  Source/Synthesis/zoscillator.cpp
  Source/Utility/dcblock.cpp
  Source/Utility/metro.cpp
)

set_target_properties(DaisySP PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
)

# Workaround: MSVC doesn’t define M_E
if (MSVC)
  target_compile_definitions(DaisySP PUBLIC _USE_MATH_DEFINES)
endif()

# ✅ Add full include paths for headers like dsp.h, Filters/svf.h etc.
target_include_directories(DaisySP PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/Source
  ${CMAKE_CURRENT_LIST_DIR}/Source/Control
  ${CMAKE_CURRENT_LIST_DIR}/Source/Drums
  ${CMAKE_CURRENT_LIST_DIR}/Source/Dynamics
  ${CMAKE_CURRENT_LIST_DIR}/Source/Effects
  ${CMAKE_CURRENT_LIST_DIR}/Source/Filters
  ${CMAKE_CURRENT_LIST_DIR}/Source/Noise
  ${CMAKE_CURRENT_LIST_DIR}/Source/PhysicalModeling
  ${CMAKE_CURRENT_LIST_DIR}/Source/Sampling
  ${CMAKE_CURRENT_LIST_DIR}/Source/Synthesis
  ${CMAKE_CURRENT_LIST_DIR}/Source/Utility
)
