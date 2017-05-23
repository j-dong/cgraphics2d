#ifndef INCLUDED_GRAPHICS_GRAPHICS_H
#define INCLUDED_GRAPHICS_GRAPHICS_H

// don't include windows.h
// define APIENTRY to prevent glad.h from including it
#ifdef _WIN32
#define APIENTRY __stdcall
#endif

// include OpenGL libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// include PNG loading library
#include <lodepng.h>

// standard libraries
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

struct tex_opt_t {
    const char *filename;
    // used for image coordinates; convenient and allows user to
    // replace with hi-res images
    unsigned int width, height;
    bool srgb;
    bool premultiplied; // if not, we manually premultiply; set to true if opaque
    bool mipmaps; // ignored unless ENABLE_MIPMAPS is defined
};

typedef struct tex_opt_t TextureOptions;

#endif
