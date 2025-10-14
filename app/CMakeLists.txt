add_executable(AVA_C
    main.cpp
)

# Make sure SDL doesn't redefine main()
target_compile_definitions(AVA_C PRIVATE SDL_MAIN_HANDLED)

# --- 🔹 Find and link libcurl ---
find_package(CURL REQUIRED)

# Link against all internal modules + SDL/OpenGL + CURL
target_link_libraries(AVA_C
    ava_core
    ava_ui
    ava_audio
    ava_dsp
    SDL2::SDL2
    opengl32
    CURL::libcurl       # ✅ Added line
)

# Auto-copy SDL2.dll next to AVA_C.exe after build
add_custom_command(TARGET AVA_C POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t "$<TARGET_FILE_DIR:AVA_C>" $<TARGET_RUNTIME_DLLS:AVA_C>
    COMMAND_EXPAND_LISTS
)

# Allow includes like #include "ui/Button.h"
target_include_directories(AVA_C PRIVATE
    ${CMAKE_SOURCE_DIR}   # gives you root: C:/AVA-C
)

# Tell the compiler where to find json.hpp
target_include_directories(AVA_C PRIVATE
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/nlohmann
)
