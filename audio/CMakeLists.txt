add_library(ava_audio STATIC
    AudioEngine.cpp
)

# include dirs so AudioEngine can see Key.h + nanovg.h + DaisySP
target_include_directories(ava_audio
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}             # audio/
        ${CMAKE_SOURCE_DIR}/ui                  # for UI.h, Key.h
        ${CMAKE_SOURCE_DIR}/external/nanovg/src # for nanovg.h
        ${CMAKE_SOURCE_DIR}/DaisySP/Source      # for DaisySP headers
)

# link with RtAudio, DSP, UI, DaisySP
target_link_libraries(ava_audio
    PUBLIC
        RtAudio::rtaudio
        ava_dsp
        ava_ui
        DaisySP
)
