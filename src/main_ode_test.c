// raylib 5.5
// flecs v4.1.1
#define RAYGUI_IMPLEMENTATION
#include <stdio.h>
#include "ecs_components.h"
#include "module_dev.h"
// #include "module_enet.h"
#include "module_ode.h"

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
