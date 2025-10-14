// raylib 5.5
// flecs v4.1.1
#define RAYGUI_IMPLEMENTATION
#include <stdio.h>
#include "ecs_components.h"
#include "module_dev.h"
#include "module_enet.h"

int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    // Initialize components and phases
    module_init_raylib(world);
    module_init_dev(world);
    module_init_enet(world);

    // setup Camera 3D
    Camera3D camera = {
        .position = (Vector3){10.0f, 10.0f, 10.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };
    ecs_set_ctx(world, &camera, NULL);

    ecs_entity_t networkEntity = ecs_new(world);
    ecs_set(world, networkEntity, NetworkConfig, {
        .isServer = true,  // Or false for client
        .port = 1234,
        .maxPeers = 32,
        .address = "localhost"  // For client
    });
    ecs_set(world, networkEntity, NetworkState, {0});  // Zeros: host=NULL, flags=false

    //Loop Logic and render
    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    // Cleanup: Add to end of main (before ecs_fini)
    // if (g_host) {
    //   enet_host_destroy(g_host);
    //   enet_deinitialize();
    // }

    ecs_fini(world);
    CloseWindow();
    return 0;
}
