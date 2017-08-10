#ifndef INCLUDED_GRAPHICS_GRAPHICS_H
#define INCLUDED_GRAPHICS_GRAPHICS_H

// graphics functions
// TODO: image coordinates in pixels

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

typedef const char *GraphicsError; // NULL == 0 for no error

typedef GLFWwindow *GraphicsWindow;

// can be called after creating a window to make sure there's no error.
inline GraphicsError graphics_get_window_error(GraphicsWindow w) {
    return w == 0 ? "error creating window" : 0;
}

// activate the window's context
inline void graphics_activate_window(GraphicsWindow w) {
    glfwMakeContextCurrent(w);
}

inline GLFWwindow *graphics_get_glfw_window_(GraphicsWindow w) { return w; }

// Initialize the graphics library and create a main window, activating and initializing it.
GraphicsError graphics_init(unsigned int width, unsigned int height,
                            const char *title, size_t maximum_draw_length, GraphicsWindow *out);
// Terminate the graphics library.
// Should be called if graphics_init completed successfully.
void graphics_terminate(void);
// Create a window without activating it.
GraphicsWindow graphics_create_window(unsigned int width, unsigned int height, const char *title);
// Destroy a window.
// (Note: this deallocates some memory we allocated in the init
// function; make sure to call this unless this is the end of the program!)
void graphics_destroy_window(GraphicsWindow w);
// Initialize a window, setting options and binding a new VAO.
// Requires an active OpenGL context.
// This allocates the internal VBOs, which are not destroyed until
// the window is destroyed.
// maximum_draw_length is the maximum number of instances that can
// be drawn with one draw call. Each instance takes up 4 GLints worth
// of memory (16 bytes).
GraphicsError graphics_init_window(GraphicsWindow w, size_t maximum_draw_length);
// Poll for incoming events. Should be called once per frame.
inline void graphics_poll_events(void) { glfwPollEvents(); }
// Check if the window has been closed.
inline int graphics_window_closed(GraphicsWindow w) { return glfwWindowShouldClose(graphics_get_glfw_window_(w)); }
// Clear the screen.
inline void graphics_clear(void) { glClear(GL_COLOR_BUFFER_BIT); }
// Draw some instances.
void graphics_draw(GraphicsWindow w, GLint *buffer, size_t length, unsigned int draw_width, unsigned int draw_height, int center_x, int center_y, float tex_width, float tex_height);
// Swap buffers
inline void graphics_end_draw(GraphicsWindow w) { glfwSwapBuffers(graphics_get_glfw_window_(w)); }

typedef GLuint GraphicsTexture;

// Load textures from files.
// Both tex_opts and textures should be tex_count-length arrays of their respective types.
GraphicsError graphics_load_textures(const TextureOptions *tex_opts, size_t tex_count, GraphicsTexture *textures);
inline void graphics_bind_texture(GraphicsTexture tex) { glBindTexture(GL_TEXTURE_2D, tex); }

#endif
