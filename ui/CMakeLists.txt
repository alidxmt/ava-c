# -----------------------------
# GLAD (C loader for OpenGL)
# -----------------------------
add_library(glad STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/glad/src/gl.c
)
set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/glad/include
)

# -----------------------------
# NanoVG GL3 backend + core
# -----------------------------
add_library(nanovg_gl3 STATIC
    nanovg_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/nanovg/src/nanovg.c
)

target_include_directories(nanovg_gl3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/nanovg
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/nanovg/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/glad/include
)

# Link GLAD so GLuint and GL symbols are consistent
target_link_libraries(nanovg_gl3 PUBLIC glad)

# -----------------------------
# UI library (C++) — your widgets
# -----------------------------
add_library(ava_ui
    UI.cpp
    UI.h
    Key.h      # 👈 header-only, safe to list
)

# Expose ui/ folder headers (Label.h, Widget.h, Key.h, etc.)
target_include_directories(ava_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/nanovg/src   # 👈 so nanovg.h is visible
)

# Link dependencies: GLAD + NanoVG + SDL2
target_link_libraries(ava_ui PUBLIC
    glad
    nanovg_gl3
    SDL2::SDL2
)
