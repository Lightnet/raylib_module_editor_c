// raylib 5.5
// flecs v4.1.1
// #define RAYGUI_IMPLEMENTATION
// #define ENET_IMPLEMENTATION

#include <stdio.h>
#include "ecs_components.h"
#include "raylib.h"
#include "module_dev.h"
#include "module_enet.h"

// draw raylib grid
void render_3d_grid(ecs_iter_t *it){
    DrawGrid(10, 1.0f);
}
// check number clients for server and client.
void render_2d_network(ecs_iter_t *it){
    ecs_query_t *q = ecs_query(it->world, {
        .terms = {
            { .id = ecs_id(enet_client_t) }
        }
    });
    int count = 0;
    ecs_iter_t qit = ecs_query_iter(it->world, q);
    while (ecs_query_next(&qit)) {
        count += qit.count;
    }
    //clean up query
    ecs_query_fini(q);
    DrawText( TextFormat("Client(s): %d", count), 2, 25*3, 20, DARKGRAY);
}

int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    // Initialize components and phases
    module_init_raylib(world);
    module_init_dev(world);
    module_init_enet(world);

    ECS_SYSTEM(world, render_3d_grid, RLRender3DPhase);


    ECS_SYSTEM(world, render_2d_network, RLRender2D1Phase);


    // setup Camera 3D
    Camera3D camera = {
        .position = (Vector3){10.0f, 10.0f, 10.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };
    ecs_singleton_set(world, main_context_t, {
        .camera = camera
    });

    // Initialize singletons with default values
    ecs_singleton_set(world, NetworkConfig, {
        .isNetwork = false,
        .isServer = false,
        .port = 1234,
        .maxPeers = 32,
        .address = "127.0.0.1"
    });
    // from input test.
    ecs_singleton_set(world, NetworkState, {
        .host = NULL,
        .peer = NULL,
        .serverStarted = false,
        .clientConnected = false,
        .isConnected = false,
        .isServer = false
    });

    //Loop Logic and render
    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    // Cleanup

    // Cleanup: Add to end of main (before ecs_fini)
    NetworkState *state = ecs_singleton_get_mut(world, NetworkState);
    if (state && state->host) {
        enet_host_destroy(state->host);
        enet_deinitialize();
    }

    ecs_fini(world);
    CloseWindow();
    return 0;
}
