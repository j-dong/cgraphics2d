#include "graphics.h"
#include "collision.h"
#include <assert.h>
#include <math.h>

const unsigned int WIDTH = 800, HEIGHT = 800;

enum texture_t {
    TEX_TEST,
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

// TODO: callbacks (resize and error)

int main(void) {
    // rendered objects
    const int map_x = 0, map_y = 0;
    const unsigned int tex_rows = 4, tex_cols = 4;
    const unsigned int tile_width = 64, tile_height = 64;
    const unsigned int map[4][4] = {
        {8, 2, 8, 8},
        {2, 8, 8, 8},
        {8, 8, 1, 1},
        {1, 1, 1, 1},
    };
    const int map_width = sizeof map[0] / sizeof map[0][0],
              map_height = sizeof map / sizeof map[0];
    // 4 int/floats per map value
    const unsigned int instance_count = sizeof map / sizeof map[0][0];
    GLint instances[4 * sizeof map / sizeof map[0][0]];
    for (unsigned int y = 0; y < sizeof map / sizeof map[0]; y++) {
        for (unsigned int x = 0; x < sizeof map[0] / sizeof map[0][0]; x++) {
            unsigned int val = map[y][x];
            unsigned int tx = val % tex_rows, ty = val / tex_rows;
            unsigned int i = 4 * (x + y * sizeof map[0] / sizeof map[0][0]);
            instances[i + 0] = x * tile_width + map_x;
            instances[i + 1] = y * tile_height + map_y;
            ((GLfloat *)instances)[i + 2] = tx / (GLfloat) tex_cols;
            ((GLfloat *)instances)[i + 3] = ty / (GLfloat) tex_rows;
        }
    }
    double player_pos[2] = {0.0, -20.0};
    double player_vel[2] = {0.0, 0.0};
    bool player_onground = false;
    const double gravity = 0.5;
    const double friction = 0.1;
    const double key_acceleration = 0.2;
    const double jump_vel = 10.0;
    const double timestep = 1.0;
    const unsigned int player_width = 16, player_height = 16;
    GLint player_data[4];
    ((GLfloat *)player_data)[2] = 0.75f;
    ((GLfloat *)player_data)[3] = 0.0f;
    GraphicsError err;
    GraphicsWindow window;
    err = graphics_init(WIDTH, HEIGHT, "Graphics", instance_count, &window);
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
        graphics_draw(window, instances, instance_count, tile_width, tile_height, 0, 0, 1.0f / tex_cols, 1.0f / tex_rows);
        double player_acc[2] = {0.0, 0.0};
        double player_delta[2] = {0.0, 0.0};
        player_acc[1] = gravity;
        player_acc[0] = key_acceleration * ((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS));
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && player_onground) {
            player_vel[1] -= jump_vel;
        }
        double fric_acc = -copysign(friction, player_vel[0]);
        double fric_t = -player_vel[0] / (fric_acc + player_acc[0]);
        if (player_vel[0] == 0 || !player_onground) {
            // update position and velocity in x
            player_delta[0] += 0.5 * player_acc[0] * timestep * timestep + player_vel[0] * timestep;
            player_vel[0] += player_acc[0] * timestep;
        } else if (fric_t >= 1.0 || fric_t < 0.0) {
            // velocity doesn't cross zero; don't need to change sign
            player_acc[0] += fric_acc;
            // update position and velocity in x
            player_delta[0] += 0.5 * player_acc[0] * timestep * timestep + player_vel[0] * timestep;
            player_vel[0] += player_acc[0] * timestep;
        } else {
            player_delta[0] += 0.5 * (player_acc[0] + fric_acc) * fric_t * fric_t + player_vel[0] * fric_t;
            // velocity is zero at this point
            // for rest of timestep, reset fric_acc
            fric_acc = player_acc[0] == 0 ? 0 : -copysign(fmin(friction, fabs(player_acc[0])), player_acc[0]);
            double rest_t = timestep - fric_t;
            player_delta[0] += 0.5 * (player_acc[0] + fric_acc) * rest_t * rest_t;
            player_vel[0] = (player_acc[0] + fric_acc) * rest_t;
        }
        // update position and velocity in y
        player_delta[1] += 0.5 * player_acc[1] * timestep * timestep + player_vel[1] * timestep;
        player_vel[1] += player_acc[1] * timestep;
        // set to true if we collide with north edge
        player_onground = false;
        for (int i = 0; i < 2; i++) {
            // collide up to two times
            AABB bounds;
            aabb_init_bounding(&bounds, player_pos[0], player_pos[1], player_delta[0], player_delta[1], player_width, player_height);
            int start_x = bounds.x1 > 0 ? bounds.x1 / (int)tile_width : 0,
                start_y = bounds.y1 > 0 ? bounds.y1 / (int)tile_height : 0,
                end_x = bounds.x2 / (int)tile_width < map_width ? bounds.x2 / (int)tile_width : (map_width - 1),
                end_y = bounds.y2 / (int)tile_height < map_height ? bounds.y2 / (int)tile_height : (map_height - 1);
            double t = 2.0;
            int edge;
            int new_pos;
            for (int y = start_y; y <= end_y; y++) {
                for (int x = start_x; x <= end_x; x++) {
                    if (map[y][x] == 8) continue;
                    double cur_t;
                    AABBEdge cur_edge;
                    AABB rect;
                    aabb_init(&rect, x * tile_width + map_x, y * tile_height + map_y,
                                     (x + 1) * tile_width + map_x, (y + 1) * tile_height + map_y);
                    cur_t = aabb_sweep(&rect, player_pos[0], player_pos[1], 1.0 / player_delta[0], 1.0 / player_delta[1], player_width, player_height, &cur_edge);
                    if (cur_t >= 0 && cur_t < t) {
                        t = cur_t;
                        edge = cur_edge;
                        union aabb_hack_t {
                            AABB aabb;
                            int coords[4];
                        } *aabb_hack = (void *)&rect;
                        assert(sizeof *aabb_hack == sizeof rect);
                        new_pos = aabb_hack->coords[edge] - !(edge >> 1) * (edge & 1 ? player_height : player_width);
                    }
                }
            }
            if (t >= 0 && t < 1.0) {
                player_pos[!(edge & 1)] += player_delta[!(edge & 1)] * t;
                player_pos[edge & 1] = new_pos;
                player_delta[edge & 1] = 0;
                player_vel[edge & 1] = 0;
                player_delta[!(edge & 1)] *= 1.0 - t;
                if (edge == EDGE_NORTH) player_onground = true;
            } else {
                for (int i = 0; i < 2; i++)
                    player_pos[i] += player_delta[i];
                break;
            }
        }
        for (int i = 0; i < 2; i++)
            player_data[i] = (int)player_pos[i];
        graphics_draw(window, player_data, 1, player_width, player_height, 0, 0, 1.0f / tex_cols, 1.0f / tex_rows);
        graphics_end_draw(window);
    }
    // cleanup
    graphics_destroy_window(window);
    graphics_terminate();
    return EXIT_SUCCESS;
}
