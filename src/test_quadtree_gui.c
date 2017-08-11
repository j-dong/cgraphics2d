#include "collision.h"
#include "graphics.h"

const unsigned int WIDTH = 512, HEIGHT = 512;

#ifndef NUM_BOXES
#define NUM_BOXES 2048
#endif
#ifndef RAND_SEED
#define RAND_SEED 42
#endif
#ifndef BOX_SIZE
#define BOX_SIZE 8
#endif

struct box_t {
    AABB aabb;
    unsigned int idx;
};

typedef struct box_t Box;

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

static void randomize(Box *boxes) {
    srand(rand());
    for (int i = 0; i < NUM_BOXES; i++) {
        int x1 = rand() % (WIDTH - BOX_SIZE) - (WIDTH - BOX_SIZE) / 2,
            y1 = rand() % (HEIGHT - BOX_SIZE) - (HEIGHT - BOX_SIZE) / 2;
        aabb_init(&boxes[i].aabb, x1, y1, x1 + BOX_SIZE, y1 + BOX_SIZE);
    }
}

static void highlight(void *box_data_, void *box_) {
    Box *box = box_;
    GLfloat *box_data = box_data_;
    box_data[4 * box->idx + 2] = 0.25f;
    box_data[4 * box->idx + 3] = 0.00f;
}

int main(void) {
    srand(RAND_SEED);
    AABB bounds;
    aabb_init(&bounds, -(int)WIDTH / 2, -(int)HEIGHT / 2, WIDTH / 2, HEIGHT / 2);
    Quadtree q;
    quadtree_init(&q, &bounds, 7, sizeof(Box));
    // objects
    AABB player;
    aabb_init(&player, 0, 0, 16, 16);
    Box *boxes = malloc(NUM_BOXES * sizeof(Box));
    for (int i = 0; i < NUM_BOXES; i++) {
        boxes[i].idx = i;
    }
    randomize(boxes);
    // 4 int/floats per map value
    GLint player_data[4], *box_data = malloc(NUM_BOXES * 4 * sizeof(GLint));
    // x, y, tx, ty
    ((GLfloat *)player_data)[2] = 0.00f;
    ((GLfloat *)player_data)[3] = 0.25f;
    for (size_t i = 0; i < NUM_BOXES; i++) {
        box_data[i * 4    ] = boxes[i].aabb.x1;
        box_data[i * 4 + 1] = boxes[i].aabb.y1;
        fflush(stdout);
        quadtree_insert(&q, &boxes[i]);
    }
    // setup
    GraphicsError err;
    GraphicsWindow window;
    // we're only drawing one thing at a time
    err = graphics_init(WIDTH, HEIGHT, "Quadtree Test", NUM_BOXES, &window);
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
        for (size_t i = 0; i < NUM_BOXES; i++) {
            box_data[i * 4 + 2] = 0.75f;
            box_data[i * 4 + 3] = 0.50f;
        }
        // highlight colliding boxes
        quadtree_traverse(&q, &player, highlight, box_data);
        // draw boxes and player
        player_data[0] = player.x1;
        player_data[1] = player.y1;
        graphics_draw(window, player_data, 1, player.x2 - player.x1, player.y2 - player.y1, 0, 0, 0.25f, 0.25f);
        graphics_draw(window, box_data, NUM_BOXES, BOX_SIZE, BOX_SIZE, 0, 0, 0.25f, 0.25f);
        // player movement
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            player.y1 -= 1;
            player.y2 -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            player.y1 += 1;
            player.y2 += 1;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            player.x1 -= 1;
            player.x2 -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            player.x1 += 1;
            player.x2 += 1;
        }
        // resizing
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            player.y2 -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            player.y2 += 1;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            player.x2 -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            player.x2 += 1;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            player.x2 = player.x1 + 16;
            player.y2 = player.y1 + 16;
        }
        graphics_end_draw(window);
    }
    // cleanup
    graphics_destroy_window(window);
    graphics_terminate();
    free(box_data);
    free(boxes);
    return EXIT_SUCCESS;
}
