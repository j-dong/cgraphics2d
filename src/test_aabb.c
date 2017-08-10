#include "collision.h"
#include "graphics.h"

const unsigned int WIDTH = 400, HEIGHT = 300;

enum texture_t {
    TEX_COLORS,
    TEX_COUNT
};

typedef enum texture_t Texture;

static const TextureOptions texture_options[TEX_COUNT] = {
    {
        "colors.png",
        64, 64,
        true,
        true,
        false,
    },
};

int main(void) {
    // objects
    const unsigned int player_width = 20, player_height = 30;
    AABB a, b, bound;
    int move[2] = {0, 0};
    aabb_init(&a, -50, -20, 50, 20);
    aabb_init(&b, -70, -40, 0, 0);
    b.x2 = b.x1 + player_width;
    b.y2 = b.y1 + player_height;
    // 4 int/floats per map value
    GLint a_data[4], b_data[4], ind_data[4], move_data[4], col_data[4], bound_data[4], edge_data[4];
    // x, y, tx, ty
    ((GLfloat *)a_data)[2] = 0.00f;
    ((GLfloat *)a_data)[3] = 0.00f;
    ((GLfloat *)b_data)[2] = 0.25f;
    ((GLfloat *)b_data)[3] = 0.00f;
    ((GLfloat *)move_data)[2] = 0.50f;
    ((GLfloat *)move_data)[3] = 0.25f;
    ((GLfloat *)col_data)[2] = 0.00f;
    ((GLfloat *)col_data)[3] = 0.25f;
    ((GLfloat *)bound_data)[2] = 0.00f;
    ((GLfloat *)bound_data)[3] = 0.75f;
    ((GLfloat *)edge_data)[2] = 0.75f;
    ((GLfloat *)edge_data)[3] = 0.00f;
    ind_data[0] = WIDTH / 2 - 16;
    ind_data[1] = -(int)HEIGHT / 2;
    GLfloat *ind_tex = (GLfloat *)&ind_data[2];
    // setup
    GraphicsError err;
    GraphicsWindow window;
    // we're only drawing one thing at a time
    err = graphics_init(WIDTH, HEIGHT, "AABB Test", 1, &window);
    if (err) {
        fputs(err, stderr);
        fputc('\n', stderr);
        return EXIT_FAILURE;
    }
    GraphicsTexture textures[TEX_COUNT];
    err = graphics_load_textures(texture_options, TEX_COUNT, textures);
    if (err) {
        fputs(err, stderr);
        fputc('\n', stderr);
        graphics_destroy_window(window);
        graphics_terminate();
        return EXIT_FAILURE;
    }
    // main loop
    while (!graphics_window_closed(window)) {
        graphics_poll_events();
        graphics_clear();
        // update data
        // bounding box
        aabb_init_bounding(&bound, b.x1, b.y1, move[0], move[1], player_width, player_height);
        bound_data[0] = bound.x1;
        bound_data[1] = bound.y1;
        graphics_draw(window, bound_data, 1, bound.x2 - bound.x1, bound.y2 - bound.y1, 0, 0, 0.25f, 0.25f);
        a_data[0] = a.x1;
        a_data[1] = a.y1;
        graphics_draw(window, a_data, 1, a.x2 - a.x1, a.y2 - a.y1, 0, 0, 0.25f, 0.25f);
        move_data[0] = b.x1 + move[0];
        move_data[1] = b.y1 + move[1];
        graphics_draw(window, move_data, 1, player_width, player_height, 0, 0, 0.25f, 0.25f);
        b_data[0] = b.x1;
        b_data[1] = b.y1;
        b.x2 = b.x1 + player_width;
        b.y2 = b.y1 + player_height;
        graphics_draw(window, b_data, 1, player_width, player_height, 0, 0, 0.25f, 0.25f);
        // blue = not touching
        // yellow = intersecting
        // orange = containing but not intersecting (error!)
        // cyan = containing
        ind_tex[0] = 0.25f * (aabb_intersect(&a, &b) | aabb_contains(&a, &b) << 1);
        ind_tex[1] = 0.25f;
        graphics_draw(window, ind_data, 1, 16, 16, 0, 0, 0.25f, 0.25f);
        // sweep test
        AABBEdge edge;
        double sweep = aabb_sweep(&a, b.x1, b.y1, 1.0 / move[0], 1.0 / move[1], player_width, player_height, &edge);
        if (sweep >= 0) {
            col_data[0] = b.x1 + (int)(move[0] * sweep);
            col_data[1] = b.y1 + (int)(move[1] * sweep);
            graphics_draw(window, col_data, 1, player_width, player_height, 0, 0, 0.25f, 0.25f);
            if (edge >> 1) {
                edge_data[0] = (a.x1 + a.x2) / 2 - 2;
                edge_data[1] = edge & 1 ? a.y2 - 4 : a.y1;
            } else {
                edge_data[0] = edge & 1 ? a.x2 - 4 : a.x1;
                edge_data[1] = (a.y1 + a.y2) / 2 - 2;
            }
            graphics_draw(window, edge_data, 1, 4, 4, 0, 0, 0.25f, 0.25f);
        }
        // player movement
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            b.y1 -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            b.y1 += 1;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            b.x1 -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            b.x1 += 1;
        }
        // move data
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            move[1] -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            move[1] += 1;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            move[0] -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            move[0] += 1;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            move[0] = move[1] = 0;
        }
        graphics_end_draw(window);
    }
    // cleanup
    graphics_destroy_window(window);
    graphics_terminate();
    return EXIT_SUCCESS;
}
