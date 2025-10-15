// raylib 5.5
// flecs v4.1.1
// strange rotation for gui
// y -85 to 85 is cap else it will have strange rotate.
// note that child does not update the changes. which need parent to update.
#include <stdio.h>
#include "ecs_components.h"
#include "module_dev.h"

void on_add_entity(ecs_iter_t *it){
    printf("add entity\n");
}

void on_remove_entity(ecs_iter_t *it){

}

int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    // Initialize components and phases
    module_init_raylib(world);
    module_init_dev(world);

    // Create observer that is invoked whenever Position is set
    ecs_observer(world, {
        .query.terms = {{ ecs_id(Transform3D) }},
        .events = { EcsOnAdd },
        .callback = on_add_entity
    });

    // setup Camera 3D
    Camera3D camera = {
        .position = (Vector3){10.0f, 10.0f, 10.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };
    ecs_set_ctx(world, &camera, NULL);

    // setup Input
    ecs_singleton_set(world, PlayerInput_T, {
      .isMovementMode=true,
      .tabPressed=false
    });

    // create Model
    Model cube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

    // Create Entity
    ecs_entity_t node1 = ecs_entity(world, {
      .name = "NodeParent"
    });

    ecs_set(world, node1, Transform3D, {
        .position = (Vector3){0.0f, 0.0f, 0.0f},
        .rotation = QuaternionIdentity(),
        .scale = (Vector3){1.0f, 1.0f, 1.0f},
        .localMatrix = MatrixIdentity(),
        .worldMatrix = MatrixIdentity(),
        .isDirty = true
    });
    ecs_set(world, node1, ModelComponent, {&cube});
    
    // Create Entity to parent to NodeParent
    ecs_entity_t node2 = ecs_entity(world, {
        .name = "NodeChild",
        .parent = node1
    });
    ecs_set(world, node2, Transform3D, {
        .position = (Vector3){2.0f, 0.0f, 0.0f},
        .rotation = QuaternionIdentity(),
        .scale = (Vector3){0.5f, 0.5f, 0.5f},
        .localMatrix = MatrixIdentity(),
        .worldMatrix = MatrixIdentity(),
        .isDirty = true
    });
    ecs_set(world, node2, ModelComponent, {&cube});
    
    // Create Entity to parent to NodeParent
    ecs_entity_t node3 = ecs_entity(world, {
        .name = "Node3",
        .parent = node1
    });
    ecs_set(world, node3, Transform3D, {
        .position = (Vector3){2.0f, 0.0f, 2.0f},
        .rotation = QuaternionIdentity(),
        .scale = (Vector3){0.5f, 0.5f, 0.5f},
        .localMatrix = MatrixIdentity(),
        .worldMatrix = MatrixIdentity(),
        .isDirty = true
    });
    ecs_set(world, node3, ModelComponent, {&cube});

    // Create Entity to parent to NodeChild
    ecs_entity_t node4 = ecs_entity(world, {
        .name = "NodeGrandchild",
        .parent = node2
    });
    ecs_set(world, node4, Transform3D, {
        .position = (Vector3){1.0f, 0.0f, 1.0f},
        .rotation = QuaternionIdentity(),
        .scale = (Vector3){0.5f, 0.5f, 0.5f},
        .localMatrix = MatrixIdentity(),
        .worldMatrix = MatrixIdentity(),
        .isDirty = true
    });
    ecs_set(world, node4, ModelComponent, {&cube});

    ecs_entity_t gui = ecs_new(world);
    ecs_set_name(world, gui, "transform_gui");  // Optional: Name for debugging
    ecs_set(world, gui, TransformGUI, {
        .id = node1  // Reference the id entity
    });

    //Loop Logic and render
    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    UnloadModel(cube);
    ecs_fini(world);
    CloseWindow();
    return 0;
}
