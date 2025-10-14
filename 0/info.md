[AVA-C CONTEXT — September 2025]

Platform & Tools
- OS: Windows 11
- IDE: Visual Studio 2022 (Desktop C++)
- CMake: 4.1.0
- vcpkg: C:\vcpkg  (triplet: x64-windows)
- Generator: "Visual Studio 17 2022", x64

Project
- Root: C:\AVA-C
- Structure:
  /app        → main loop (SDL + GL + NanoVG), entry point
  /ui         → NanoVG widgets & GL3 backend lib
  /audio      → RtAudio engine wrapper (callback), UI→audio params
  /dsp        → Oscillator + basic params
  /external/glad/...
  /external/nanovg/
      nanovg.h
      nanovg_gl.h
      src/
        nanovg.c
        fontstash.h
        stb_image.h
        stb_truetype.h
  /build      → VS solution + binaries

3rd-party linkage
- Link: SDL2::SDL2, opengl32, RtAudio::rtaudio
- GL loader: GLAD 2 (OpenGL 3.3 Core)
- NanoVG GL3 backend builds as a static lib: ui/nanovg_gl3
  * File: ui/nanovg_backend.cpp
  * Includes order (important):
      #include <glad/gl.h>
      #include <nanovg.h>
      extern "C" {
        #define NANOVG_GL3_IMPLEMENTATION
        #include "nanovg_gl.h"
      }
- App TU uses:
    #include <nanovg.h>
    #define NVG_GL3 1
    #include <nanovg_gl.h>
  and declares (C linkage) in app/main.cpp:
    extern "C" {
      NVGcontext* nvgCreateGL3(int);
      void nvgDeleteGL3(NVGcontext*);
    }

Build Flags / Options
- AVA_ENABLE_FFT=OFF (kissfft temporarily disabled due to config issue)
- SDL_MAIN_HANDLED on AVA_C

CMake snippets (key parts)
- app/CMakeLists.txt links:
    ava_ui, ava_audio, ava_dsp, SDL2::SDL2, opengl32
- Post-build runtime copy (recommended, working):
    add_custom_command(TARGET AVA_C POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy -t "$<TARGET_FILE_DIR:AVA_C>" $<TARGET_RUNTIME_DLLS:AVA_C>
      COMMAND_EXPAND_LISTS)

Build & Run
- Configure:
    cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ^
      -DAVA_ENABLE_FFT=OFF
- Build:
    cmake --build build --config Debug --target AVA_C
- EXE path:
    C:\AVA-C\build\app\Debug\AVA_C.exe
- Runtime:
    SDL2.dll auto-copied by post-build rule (or copy from vcpkg if needed)

Current App Behavior (baseline)
- SDL2 creates GL 3.3 core window; GLAD loads functions; NanoVG draws a simple panel + gain bar
- RtAudio runs stereo float32 sine via callback
- Controls:
    Up/Down = semitone ±
    Left/Right = gain ±
    Esc = quit
- Multitouch events are plumbed (SDL_FINGERDOWN/MOTION) but not yet routed to widgets

Known Good Files (as of now)
- app/main.cpp (uses NVG_GL3 + extern "C" prototypes; calls nvgCreateGL3/nvgDeleteGL3)
- ui/nanovg_backend.cpp (GLAD first, extern "C" block around nanovg_gl.h implementation)
- audio/AudioEngine.cpp (catch std::exception; no RtAudioError)
- dsp/Oscillator.* (simple sine)
- ui/* (Root, GainBar, Primitives; minimal UI)

Troubleshooting Notes
- If NanoVG compile errors mention GLuint/type, ensure GLAD included before nanovg_gl.h (backend) and no duplicate NanoVG files outside external/nanovg/src.
- If linker can’t find nvgCreateGL3/nvgDeleteGL3, ensure extern "C" in backend and C-linkage prototypes visible in app/main.cpp.

Next Steps (COMBI MODE)
1) You’ll share JS/HTML/CSS for the first UI control (e.g., a gain/parameter slider or XY pad).
2) I’ll map it to a NanoVG widget in /ui, add touch handling in Root, and wire its value atomically into AudioEngine.
3) Add more DSP blocks (tremolo/chorus/reverb), then re-enable FFT (kissfft) for a spectrum panel.

