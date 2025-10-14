add_library(ava_dsp STATIC
Oscillator.cpp
)


target_include_directories(ava_dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


# Optional: expose kissfft if available (for future spectrum panel)
if(TARGET kissfft::kissfft-float)
target_link_libraries(ava_dsp PUBLIC kissfft::kissfft-float)
target_compile_definitions(ava_dsp PUBLIC AVA_HAVE_KISSFFT=1)
endif()