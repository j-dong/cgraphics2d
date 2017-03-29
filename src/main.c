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

// values specified in shader
enum vx_attributes {
    VX_POSITION  = 0,
    VXI_DRAW_POS = 1,
    VXI_TEX_POS  = 2,
};

enum uniforms {
    U_TEX = 0,
    U_WIN_SIZE = 1,
    U_DRAW_SIZE = 2,
    U_CENTER_POS = 3,
    U_TEX_SIZE = 4,
    UNIFORM_COUNT
};

const char *uniform_name[UNIFORM_COUNT] = {
    "tex",
    "win_size",
    "draw_size",
    "center_pos",
    "tex_size",
};

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

GLuint gen_shader(GLenum shader_type, const char *source) {
    GLuint shader = glCreateShader(shader_type);
    if (!shader) return 0;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint il_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &il_length);
        char *log = (char *)malloc(il_length);
        glGetShaderInfoLog(shader, il_length, &il_length, log);
        fputs(log, stderr);
        putc('\n', stderr);
        free(log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

const char \
      *v_shader_src = \
    "#version 330\n"
    "layout(location = 0) in vec2 position;\n"
    "out vec2 o_pos;\n"
    "uniform ivec2 win_size;\n"
    "uniform ivec2 draw_size;\n"
    "uniform ivec2 center_pos;\n"
    "uniform vec2 tex_size;\n"
    "layout(location = 1) in ivec2 draw_pos;\n"
    "layout(location = 2) in vec2 tex_pos;\n"
    "void main() {\n"
    "    o_pos = vec2(0.0, 1.0) + vec2(1.0, -1.0) * (tex_pos + position * tex_size);\n"
    "    vec2 pos = (vec2(draw_pos - center_pos) + vec2(draw_size) * position) * vec2(2.0, -2.0) / vec2(win_size);\n"
    "    gl_Position = vec4(pos, 0.0, 1.0);\n"
    "}\n"
    , *f_shader_src = \
    "#version 330\n"
    "layout(location = 0) out vec4 color;\n"
    "in vec2 o_pos;\n"
    "uniform sampler2D tex;\n"
    "void main() {\n"
    "    color = texture(tex, o_pos);\n"
    "}\n"
    ;

// TODO: error handling

int main(void) {
    int exit_code = EXIT_SUCCESS;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Graphics", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (!window) {
        fputs("error creating GLFW window\n", stderr);
        exit_code = EXIT_FAILURE;
        goto pre_init_error;
    }
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        fputs("error loading OpenGL\n", stderr);
        exit_code = EXIT_FAILURE;
        goto pre_init_error;
    }
    glViewport(0, 0, WIDTH, HEIGHT);
    glfwSwapInterval(1); // turn on vsync
    // bind VAO (we basically only need one VBO)
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
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
            data = NULL;
            // close immediately
            fprintf(stderr, "error loading %s: %s\n", texture_options[i].filename, lodepng_error_text(error));
            exit_code = EXIT_FAILURE;
            goto pre_init_error;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, texture_options[i].srgb ? GL_SRGB_ALPHA : GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        free(data);
#ifdef ENABLE_MIPMAPS
        if (texture_options[i].mipmaps)
            glGenerateMipmap(GL_TEXTURE_2D);
#endif
    }
    // unbind if wanted
    // glBindTexture(GL_TEXTURE_2D, 0);
    // rendered objects
    const unsigned int tex_rows = 2, tex_cols = 2;
    const unsigned int tile_width = 64, tile_height = 64;
    const unsigned int map[4][4] = {
        {0, 1, 2, 3},
        {2, 2, 2, 2},
        {1, 3, 1, 1},
        {2, 2, 0, 2},
    };
    // set up buffer data
    const GLfloat vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
    };
    // 4 int/floats per map value
    const unsigned int instance_count = sizeof map / sizeof map[0][0];
    GLint instances[4 * sizeof map / sizeof map[0][0]];
    for (unsigned int y = 0; y < sizeof map / sizeof map[0]; y++) {
        for (unsigned int x = 0; x < sizeof map[0] / sizeof map[0][0]; x++) {
            unsigned int val = map[y][x];
            unsigned int tx = val % tex_rows, ty = val / tex_rows;
            unsigned int i = 4 * (x + y * sizeof map[0] / sizeof map[0][0]);
            instances[i + 0] = x * tile_width;
            instances[i + 1] = y * tile_height;
            ((GLfloat *)instances)[i + 2] = tx / (GLfloat) tex_cols;
            ((GLfloat *)instances)[i + 3] = ty / (GLfloat) tex_rows;
        }
    }
    // allocate and register buffers
    GLuint vbo; // stores vertices of quad
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(VX_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(VX_POSITION);
    GLuint vbo_inst; // stores instance attributes
    glGenBuffers(1, &vbo_inst);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_inst);
    glBufferData(GL_ARRAY_BUFFER, sizeof instances, instances, GL_STATIC_DRAW);
    glVertexAttribIPointer(VXI_DRAW_POS, 2, GL_INT,             4 * sizeof(GLint), (void *)0);
    glVertexAttribPointer (VXI_TEX_POS,  2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLint), (void *)(2 * sizeof(GLint)));
    // per-instance attribute
    glVertexAttribDivisor(VXI_DRAW_POS, 1);
    glVertexAttribDivisor(VXI_TEX_POS,  1);
    glEnableVertexAttribArray(VXI_DRAW_POS);
    glEnableVertexAttribArray(VXI_TEX_POS);
    // unbind if wanted
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // create and bind shaders
    GLuint v_shader = gen_shader(GL_VERTEX_SHADER,   v_shader_src);
    GLuint f_shader = gen_shader(GL_FRAGMENT_SHADER, f_shader_src);
    if (v_shader == 0 || f_shader == 0) {
        glDeleteShader(v_shader);
        glDeleteShader(f_shader);
        v_shader = f_shader = 0;
        exit_code = EXIT_FAILURE;
        goto shader_error;
    }
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, v_shader);
    glAttachShader(shader_program, f_shader);
    glLinkProgram(shader_program);
    GLint success = 0;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint il_length = 0;
        glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &il_length);
        char *log = (char *)malloc(il_length);
        glGetProgramInfoLog(shader_program, il_length, &il_length, log);
        fputs(log, stderr);
        putc('\n', stderr);
        free(log);
        exit_code = EXIT_FAILURE;
        goto shader_program_error;
    }
    glDetachShader(shader_program, v_shader);
    glDetachShader(shader_program, f_shader);
    glDeleteShader(v_shader);
    glDeleteShader(f_shader);
    // uniform locations
    GLint ul[UNIFORM_COUNT];
    for (int i = 0; i < UNIFORM_COUNT; i++)
        ul[i] = glGetUniformLocation(shader_program, uniform_name[i]);
    // use program, set uniforms
    glUseProgram(shader_program);
    glUniform1i(ul[U_TEX], 0); // texture unit 0
    // if resizing desired, set this in main loop or resize handler
    glUniform2i(ul[U_WIN_SIZE], WIDTH, HEIGHT);
    // main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // set uniforms for drawing
        glUniform2i(ul[U_DRAW_SIZE], tile_width, tile_height);
        glUniform2i(ul[U_CENTER_POS], 0, 0);
        glUniform2f(ul[U_TEX_SIZE], 1.0f / tex_cols, 1.0f / tex_rows);
        // draw using bound arrays
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instance_count);
        glfwSwapBuffers(window);
    }
    // cleanup
post_init_error:
shader_program_error:
    glDeleteProgram(shader_program);
shader_error:
vbo_error:
    glDeleteBuffers(1, &vbo_inst);
    glDeleteBuffers(1, &vbo);
texture_error:
    glDeleteTextures(TEX_COUNT, textures);
vao_error:
    glDeleteVertexArrays(1, &vao);
pre_init_error:
    glfwSetWindowShouldClose(window, 1);
    glfwTerminate();
    return exit_code;
}
