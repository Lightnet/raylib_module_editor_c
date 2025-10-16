// raylib 5.5
// flecs v4.1.1
// strange rotation for gui
// y -85 to 85 is cap else it will have strange rotate.
// note that child does not update the changes. which need parent to update.
#include <stdio.h>
#include "ecs_components.h"
#include "module_dev.h"

// Define the SceneContext struct
typedef struct {
    Camera3D camera;
    // tmp physics; (assuming this is a placeholder, you can add actual physics data later)
} SceneContext;

void render_3d_grid(ecs_iter_t *it){
    DrawGrid(10, 1.0f);
}

void camera_input_system(ecs_iter_t *it){

    Camera3D *camera = (Camera3D *)ecs_get_ctx(it->world);
    if (!camera) return;
    // Camera3D *camera = &sceneContext->camera;
    if (IsCursorHidden()) UpdateCamera(camera, CAMERA_FIRST_PERSON);
    // Toggle camera controls
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        if (IsCursorHidden()) EnableCursor();
        else DisableCursor();
    }
}

int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    // Initialize components and phases
    module_init_raylib(world);
    module_init_dev(world);

    // Create observer that is invoked whenever Position is set
    // ecs_observer(world, {
    //     .query.terms = {{ ecs_id(Transform3D) }},
    //     .events = { EcsOnAdd },
    //     .callback = on_add_entity
    // });

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

    ECS_SYSTEM(world, camera_input_system, LogicUpdatePhase);
    ECS_SYSTEM(world, render_3d_grid, RLRender3DPhase);

    // setup Input
    // test
    // ecs_singleton_set(world, PlayerInput_T, {
    //   .isMovementMode=true,
    //   .tabPressed=false
    // });

    // create Model
    // Model cube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));


    //Loop Logic and render
    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    // UnloadModel(cube);
    ecs_fini(world);
    CloseWindow();
    return 0;
}
