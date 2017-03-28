#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <lodepng.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

const GLuint WIDTH = 400, HEIGHT = 300;

enum texture_t {
    TEX_TEST,
    TEX_COUNT
};

typedef enum texture_t Texture;

static const struct tex_opt_t {
    const char *filename;
    // used for image coordinates; convenient and allows user to
    // replace with hi-res images
    unsigned int width, height;
    bool srgb;
    bool mipmaps; // ignored unless ENABLE_MIPMAPS is defined
} texture_options[TEX_COUNT] = {
    {
        "test.png",
        128, 128,
        true,
        false,
    },
};

int main(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Graphics", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (!window) {
        fputs("error creating GLFW window\n", stderr);
        glfwTerminate();
        return EXIT_FAILURE;
    }
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        fputs("error loading OpenGL\n", stderr);
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glViewport(0, 0, WIDTH, HEIGHT);
    // load resources
    GLuint textures[TEX_COUNT] = {0};
    glGenTextures(TEX_COUNT, textures);
    for (int i = 0; i < TEX_COUNT; i++) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        // texture parameters (wrapping, filtering)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        unsigned char *data = NULL;
        unsigned int width, height;
        unsigned int error;
        if ((error = lodepng_decode32_file(&data, &width, &height, texture_options[i].filename))) {
            free(data);
            // close immediately
            glfwSetWindowShouldClose(window, true);
            fprintf(stderr, "error loading %s: %s\n", texture_options[i].filename, lodepng_error_text(error));
            break;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, texture_options[i].srgb ? GL_SRGB_ALPHA : GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        free(data);
#ifdef ENABLE_MIPMAPS
        if (texture_options[i].mipmaps)
            glGenerateMipmap(GL_TEXTURE_2D);
#endif
    }
    // main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }
    // cleanup
    glDeleteTextures(TEX_COUNT, textures);
    glfwTerminate();
    return EXIT_SUCCESS;
}
