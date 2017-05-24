#include "graphics.h"

const unsigned int WIDTH = 400, HEIGHT = 300;

enum texture_t {
    TEX_TEST,
    TEX_COUNT
};

typedef enum texture_t Texture;

static const TextureOptions texture_options[TEX_COUNT] = {
    {
        "test.png",
        128, 128,
        true,
        true,
        false,
    },
};

// TODO: callbacks (resize and error)

int main(void) {
    GraphicsError err;
    GraphicsWindow window;
    err = graphics_init(WIDTH, HEIGHT, "Graphics", &window);
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
    // rendered objects
    const unsigned int tex_rows = 2, tex_cols = 2;
    const unsigned int tile_width = 64, tile_height = 64;
    const unsigned int map[4][4] = {
        {0, 1, 2, 3},
        {2, 2, 2, 2},
        {1, 3, 1, 1},
        {2, 2, 0, 2},
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
    // main loop
    while (!graphics_window_closed(window)) {
        graphics_poll_events();
        graphics_clear();
        graphics_draw(window, instances, instance_count, tile_width, tile_height, 0, 0, 1.0f / tex_cols, 1.0f / tex_rows);
        graphics_end_draw(window);
    }
    // cleanup
    graphics_destroy_window(window);
    graphics_terminate();
    return EXIT_SUCCESS;
}
