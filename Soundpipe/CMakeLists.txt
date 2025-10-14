cmake_minimum_required(VERSION 3.10)
project(Soundpipe C)

# Collect all C sources in modules + lib subdirs
file(GLOB SOUNDPIPE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/*/*.c"
)

# Build static library
add_library(Soundpipe STATIC ${SOUNDPIPE_SRC})

# Public include directories
target_include_directories(Soundpipe PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/h
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# Silence MSVC warnings (optional)
if(MSVC)
    target_compile_definitions(Soundpipe PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
