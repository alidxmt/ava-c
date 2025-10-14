add_library(ava_core STATIC
    EventRouter.cpp
)

# Expose core/ for EventRouter.h
target_include_directories(ava_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against SDL2 so EventRouter can see <SDL.h>
target_link_libraries(ava_core PUBLIC
    SDL2::SDL2
)
