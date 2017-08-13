#include "graphics.h"

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

static const char *uniform_name[UNIFORM_COUNT] = {
    "tex",
    "win_size",
    "draw_size",
    "center_pos",
    "tex_size",
};

static GLuint gen_shader(GLenum shader_type, const char *source) {
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

static const char \
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
    "    o_pos = tex_pos + position * tex_size;\n"
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

static const GLfloat graphics_vertices[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
};

GraphicsError graphics_init(unsigned int width, unsigned int height, const char *title, size_t maximum_draw_length, GraphicsWindow *out) {
    if (!glfwInit()) {
        return "error initializing GLFW";
    }
    *out = graphics_create_window(width, height, title);
    GraphicsError err;
    err = graphics_get_window_error(*out);
    if (err) return err;
    graphics_activate_window(*out);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        graphics_destroy_window(*out);
        return "error loading OpenGL\n";
    }
    err = graphics_init_window(*out, maximum_draw_length);
    if (err) return err;
    return 0;
}

void graphics_terminate(void) {
    glfwTerminate();
}

GraphicsWindow graphics_create_window(unsigned int width, unsigned int height, const char *title) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    return glfwCreateWindow(width, height, title, NULL, NULL);
}

GraphicsError graphics_init_window(GraphicsWindow w, size_t maximum_draw_length) {
    int fbw, fbh, ww, wh;
    glfwGetFramebufferSize(graphics_get_glfw_window_(w), &fbw, &fbh);
    glfwGetWindowSize(graphics_get_glfw_window_(w), &ww, &wh);
    glViewport(0, 0, fbw, fbh);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(1); // turn on vsync
    // bind VAO (we basically only need one VAO)
    // we won't bother deleting it at the end
    GLuint vao;
    glGenVertexArrays(1, &vao);
    if (vao == 0) return "error initializing window";
    glBindVertexArray(vao);
    // allocate and register buffers
    GLuint vbo; // stores vertices of quad
    glGenBuffers(1, &vbo);
    if (vbo == 0) {
        glDeleteVertexArrays(1, &vao);
        return "error creating non-per-instance VBO";
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof graphics_vertices, graphics_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(VX_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(VX_POSITION);
    GLuint vbo_inst; // stores instance attributes
    glGenBuffers(1, &vbo_inst);
    if (vbo_inst == 0) {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return "error creating per-instance VBO";
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_inst);
    glBufferData(GL_ARRAY_BUFFER, maximum_draw_length * 4 * sizeof(GLint), 0, GL_STATIC_DRAW);
    glVertexAttribIPointer(VXI_DRAW_POS, 2, GL_INT,             4 * sizeof(GLint), (void *)0);
    glVertexAttribPointer (VXI_TEX_POS,  2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLint), (void *)(2 * sizeof(GLint)));
    // per-instance attribute
    glVertexAttribDivisor(VXI_DRAW_POS, 1);
    glVertexAttribDivisor(VXI_TEX_POS,  1);
    glEnableVertexAttribArray(VXI_DRAW_POS);
    glEnableVertexAttribArray(VXI_TEX_POS);
    // load shaders
    GLuint v_shader = gen_shader(GL_VERTEX_SHADER,   v_shader_src);
    GLuint f_shader = gen_shader(GL_FRAGMENT_SHADER, f_shader_src);
    if (v_shader == 0 || f_shader == 0) {
        glDeleteShader(v_shader);
        glDeleteShader(f_shader);
        glDeleteBuffers(1, &vbo_inst);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return "error creating shaders";
    }
    GLuint shader_program = glCreateProgram();
    if (shader_program == 0) {
        glDeleteShader(v_shader);
        glDeleteShader(f_shader);
        glDeleteBuffers(1, &vbo_inst);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return "error creating shaders";
    }
    glAttachShader(shader_program, v_shader);
    glAttachShader(shader_program, f_shader);
    glLinkProgram(shader_program);
    GLint success = 0;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    glDetachShader(shader_program, v_shader);
    glDetachShader(shader_program, f_shader);
    glDeleteShader(v_shader);
    glDeleteShader(f_shader);
    if (success == GL_FALSE) {
        GLint il_length = 0;
        glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &il_length);
        char *log = (char *)malloc(il_length);
        glGetProgramInfoLog(shader_program, il_length, &il_length, log);
        fputs(log, stderr);
        putc('\n', stderr);
        free(log);
        glDeleteBuffers(1, &vbo_inst);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return "error creating shaders";
    }
    // uniform locations
    GLint *ul = malloc(sizeof(GLint) * (UNIFORM_COUNT + 1));
    if (!ul) {
        glDeleteBuffers(1, &vbo_inst);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return "error allocating memory for uniform locations";
    }
    for (int i = 0; i < UNIFORM_COUNT; i++)
        ul[i] = glGetUniformLocation(shader_program, uniform_name[i]);
    // also store the instance VBO
    ((GLuint *)ul)[UNIFORM_COUNT] = vbo_inst;
    // use program, set uniforms
    glUseProgram(shader_program);
    glUniform1i(ul[U_TEX], 0); // texture unit 0
    // if resizing desired, set this in main loop or resize handler
    glUniform2i(ul[U_WIN_SIZE], ww, wh);
    // set uniforms to the user pointer
    glfwSetWindowUserPointer(graphics_get_glfw_window_(w), ul);
    return 0;
}

GraphicsError graphics_load_textures(const TextureOptions *tex_opts, size_t tex_count, GraphicsTexture *textures) {
    glGenTextures(tex_count, textures);
    for (unsigned int i = 0; i < tex_count; i++) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        // texture parameters (wrapping, filtering)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        unsigned char *data = NULL;
        unsigned int width, height;
        unsigned int error;
        if ((error = lodepng_decode32_file(&data, &width, &height, tex_opts[i].filename))) {
            free(data);
            data = NULL;
            // close immediately
            fprintf(stderr, "error loading %s: %s\n", tex_opts[i].filename, lodepng_error_text(error));
            glDeleteTextures(tex_count, textures);
            return "error loading textures";
        }
        // premultiply if needed
        if (!tex_opts[i].premultiplied) {
            size_t j = 4 * (width * height - 1);
            for (;; j -= 4) {
                unsigned short alpha = (unsigned short)data[j + 3] + 1;
                data[j + 0] = (unsigned char)((alpha * data[j + 0]) / 256);
                data[j + 1] = (unsigned char)((alpha * data[j + 1]) / 256);
                data[j + 2] = (unsigned char)((alpha * data[j + 2]) / 256);
                if (j == 0) break;
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, tex_opts[i].srgb ? GL_SRGB_ALPHA : GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        free(data);
#ifdef ENABLE_MIPMAPS
        if (tex_opts[i].mipmaps)
            glGenerateMipmap(GL_TEXTURE_2D);
#endif
    }
    return 0;
}

void graphics_destroy_window(GraphicsWindow w) {
    free(glfwGetWindowUserPointer(graphics_get_glfw_window_(w)));
    glfwDestroyWindow(graphics_get_glfw_window_(w));
}

void graphics_draw(GraphicsWindow w, GLint *buffer, size_t length, unsigned int draw_width, unsigned int draw_height, int center_x, int center_y, float tex_width, float tex_height) {
    GLint *ul = glfwGetWindowUserPointer(graphics_get_glfw_window_(w));
    GLuint vbo = ((GLuint *)ul)[UNIFORM_COUNT];
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, length * 4 * sizeof(GLint), buffer);
    // set uniforms for drawing
    glUniform2i(ul[U_DRAW_SIZE], draw_width, draw_height);
    glUniform2i(ul[U_CENTER_POS], center_x, center_y);
    glUniform2f(ul[U_TEX_SIZE], tex_width, tex_height);
    // draw using bound arrays
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, length);
}

#ifndef __cplusplus
// definitions of inline functions
GraphicsError graphics_get_window_error(GraphicsWindow w);
void graphics_activate_window(GraphicsWindow w);
GLFWwindow *graphics_get_glfw_window_(GraphicsWindow w);
void graphics_poll_events(void);
int graphics_window_closed(GraphicsWindow w);
void graphics_clear(void);
void graphics_end_draw(GraphicsWindow w);
void graphics_bind_texture(GraphicsTexture tex);
#endif
