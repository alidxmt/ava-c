#include <glad/gl.h>    // GL types first
#include <nanovg.h>     // NVGcontext, etc.

extern "C" {
  #define NANOVG_GL3_IMPLEMENTATION
  #include "nanovg_gl.h"  // compiles the GL3 backend here with C linkage
}
