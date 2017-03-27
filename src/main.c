#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>

const GLuint WIDTH = 400, HEIGHT = 300;

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
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return EXIT_SUCCESS;
}
