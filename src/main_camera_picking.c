// raylib 5.5
// flecs v4.1.1
// strange rotation for gui
// y -85 to 85 is cap else it will have strange rotate.
// note that child does not update the changes. which need parent to update.
#include <stdio.h>
#include "ecs_components.h"
#include "module_dev.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MOUSE_SENSITIVITY 0.002f
#define MOVE_SPEED 5.0f
#define SPRINT_MULTIPLIER 2.0f
#define CAMERA_FOV 60.0f
#define CAMERA_MIN_DISTANCE 0.1f
#define MAX_PITCH 89.0f * DEG2RAD

// Define the name_t struct
typedef struct {
    float yaw;
    float pitch;
} camera_controller_t;
ECS_COMPONENT_DECLARE(camera_controller_t);

typedef struct {
    Vector3 position;
    float width;
    float height;
    float length;
    Color color;
} cube_wire_t;
ECS_COMPONENT_DECLARE(cube_wire_t);

// draw raylib grid
void render_3d_grid(ecs_iter_t *it){
    DrawGrid(10, 1.0f);
}

// camera input
void camera_input_system(ecs_iter_t *it){
    main_context_t *main_context = ecs_field(it, main_context_t, 0);
    camera_controller_t *camera_controller = ecs_field(it, camera_controller_t, 1);

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        if (IsCursorHidden()) EnableCursor();
        else DisableCursor();
    }

    if(IsCursorHidden()){
        // printf("...\n");
        // Get mouse delta (captured mouse movement)
        Vector2 mouseDelta = GetMouseDelta();
        
        // Only update rotation when mouse is captured
        if (IsWindowFocused())
        {
            // Update yaw and pitch from mouse delta
            camera_controller->yaw += mouseDelta.x * MOUSE_SENSITIVITY;
            camera_controller->pitch -= mouseDelta.y * MOUSE_SENSITIVITY;

            // Clamp pitch to prevent flipping
            camera_controller->pitch = Clamp(camera_controller->pitch, -MAX_PITCH, MAX_PITCH);

            // printf("pitch:%f yaw:%f\n", camera_controller->pitch, camera_controller->yaw);
        }

        // Calculate forward direction using spherical coordinates
        Vector3 forward;
        forward.x = cosf(camera_controller->yaw) * cosf(camera_controller->pitch);
        forward.y = sinf(camera_controller->pitch);
        forward.z = sinf(camera_controller->yaw) * cosf(camera_controller->pitch);
        forward = Vector3Normalize(forward);

        // Update camera target
        main_context->camera.target = Vector3Add(main_context->camera.position, forward);

        // Movement input
        Vector3 velocity = { 0.0f, 0.0f, 0.0f };
        float speed = MOVE_SPEED;

        // Sprint
        if (IsKeyDown(KEY_LEFT_SHIFT))
            speed *= SPRINT_MULTIPLIER;

        // Get normalized forward direction (horizontal)
        Vector3 forwardFlat = forward;
        forwardFlat.y = 0.0f;
        forwardFlat = Vector3Normalize(forwardFlat);

        // Get right vector
        Vector3 right = Vector3CrossProduct(forwardFlat, main_context->camera.up);
        right = Vector3Normalize(right);

        // WASD movement
        if (IsKeyDown(KEY_W))
            velocity = Vector3Add(velocity, Vector3Scale(forwardFlat, speed));
        if (IsKeyDown(KEY_S))
            velocity = Vector3Subtract(velocity, Vector3Scale(forwardFlat, speed));
        if (IsKeyDown(KEY_A))
            velocity = Vector3Subtract(velocity, Vector3Scale(right, speed));
        if (IsKeyDown(KEY_D))
            velocity = Vector3Add(velocity, Vector3Scale(right, speed));

        // Vertical movement (spectator mode)
        if (IsKeyDown(KEY_SPACE))
            velocity.y += speed;
        if (IsKeyDown(KEY_LEFT_CONTROL))
            velocity.y -= speed;

        // Apply movement
        Vector3 movement = Vector3Scale(velocity, GetFrameTime());
        main_context->camera.position = Vector3Add(main_context->camera.position, movement);
        main_context->camera.target = Vector3Add(main_context->camera.target, movement);

        // Keep cursor centered
        SetMousePosition(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    }

    // Camera3D *camera = &sceneContext->camera;
    // if (IsCursorHidden()) UpdateCamera(&camera, CAMERA_FIRST_PERSON);
    // if (IsCursorHidden()) UpdateCamera(&main_context->camera, CAMERA_FIRST_PERSON);
    // // Toggle camera controls
    // if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    // {
    //     if (IsCursorHidden()) EnableCursor();
    //     else DisableCursor();
    // }
}

// render 3d draw cube wires
void render_3d_draw_cube_wires(ecs_iter_t *it){
    cube_wire_t *cube_wire = ecs_field(it, cube_wire_t, 0);
    // Iterate matched entities
    for (int i = 0; i < it->count; i++) {
        DrawCubeWires(cube_wire[i].position, cube_wire[i].width,cube_wire[i].height,cube_wire[i].length,cube_wire[i].color);
    }
}

// picking raycast
void cube_wires_picking_system(ecs_iter_t *it){
    main_context_t *main_context = ecs_field(it, main_context_t, 0);
    cube_wire_t *cube_wire = ecs_field(it, cube_wire_t, 1);

    Ray ray = { 0 };                    // Picking line ray
    RayCollision collision = { 0 };     // Ray collision hit info


    ray = GetScreenToWorldRay(GetMousePosition(), main_context->camera);

    // Iterate matched entities
    for (int i = 0; i < it->count; i++) {
        DrawCubeWires(cube_wire[i].position, cube_wire[i].width,cube_wire[i].height,cube_wire[i].length, cube_wire[i].color);
        collision = GetRayCollisionBox(ray,
            (BoundingBox){(Vector3){ cube_wire[i].position.x - cube_wire[i].width/2, cube_wire[i].position.y - cube_wire[i].height/2, cube_wire[i].position.z - cube_wire[i].length/2 },
                          (Vector3){ cube_wire[i].position.x + cube_wire[i].width/2, cube_wire[i].position.y + cube_wire[i].height/2, cube_wire[i].position.z + cube_wire[i].length/2 }} );
        if (collision.hit){
            cube_wire[i].color = GREEN;
        }else{
            cube_wire[i].color = GRAY;
        }
    }


}


void render_2d_draw_cross_point(ecs_iter_t *it){
    DrawCircleLines((int)(WINDOW_WIDTH/2), (int)(WINDOW_HEIGHT/2), 8, DARKBLUE);
}

// 
int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    ECS_COMPONENT_DEFINE(world, camera_controller_t);
    ECS_COMPONENT_DEFINE(world, cube_wire_t);

    // Initialize components and phases
    module_init_raylib(world);
    module_init_dev(world);

    ECS_SYSTEM(world, render_3d_grid, RLRender3DPhase);

    // Create observer that is invoked whenever Position is set
    // ecs_observer(world, {
    //     .query.terms = {{ ecs_id(Transform3D) }},
    //     .events = { EcsOnAdd },
    //     .callback = on_add_entity
    // });

    // does not work incorrect config
    // ECS_SYSTEM(world, camera_input_system, LogicUpdatePhase, main_context_t, camera_controller_t);
    // Input for camera
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "network_input_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) }, // Singleton
            { .id = ecs_id(camera_controller_t), .src.id = ecs_id(camera_controller_t) }   // Singleton
        },
        .callback = camera_input_system
    });

    // Render 3D Cube
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_3d_draw_cube_wires", .add = ecs_ids(ecs_dependson(RLRender3DPhase)) }),
        .query.terms = {
            { .id = ecs_id(cube_wire_t) }, //
            // { .id = ecs_id(camera_controller_t), .src.id = ecs_id(camera_controller_t) }   // Singleton
        },
        .callback = render_3d_draw_cube_wires
    });

    // draw cube wireframe
    ecs_system(world,{
        .entity = ecs_entity(world, { .name = "render_3d_draw_cube_wires", .add = ecs_ids(ecs_dependson(RLRender3DPhase)) }),
        .query.terms = {
            { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) }   // Singleton
        },
        .callback = render_3d_draw_cube_wires
    });

    // picking raycast system
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "cube_wires_picking_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) }, // Singleton
            { .id = ecs_id(cube_wire_t) }   //
        },
        .callback = cube_wires_picking_system
    });

    // draw center
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_2d_draw_cross_point", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
        .callback = render_2d_draw_cross_point
    });



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

    ecs_singleton_set(world, camera_controller_t, {
        .yaw = 22.792f,
        .pitch = -0.592f,
    });


    // setup Input
    // ecs_singleton_set(world, PlayerInput_T, {
    //   .isMovementMode=true,
    //   .tabPressed=false
    // });

    ecs_entity_t cube_wired_1 = ecs_entity(world, {
      .name = "CubeWire1"
    });
    ecs_set(world, cube_wired_1, cube_wire_t, {
        .position = (Vector3) {0,0,0},
        .width = 1.0f,
        .height = 1.0f,
        .length = 1.0f,
        .color = BLUE
    });


    ecs_entity_t cube_wired_2 = ecs_entity(world, {
      .name = "CubeWire2"
    });
    ecs_set(world, cube_wired_2, cube_wire_t, {
        .position = (Vector3) {0,2,0},
        .width = 1.0f,
        .height = 1.0f,
        .length = 1.0f,
        .color = RED
    });


    // create Model
    // Model cube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

    // Create Entity
    // ecs_entity_t node1 = ecs_entity(world, {
    //   .name = "NodeParent"
    // });

    // ecs_set(world, node1, Transform3D, {
    //     .position = (Vector3){0.0f, 0.0f, 0.0f},
    //     .rotation = QuaternionIdentity(),
    //     .scale = (Vector3){1.0f, 1.0f, 1.0f},
    //     .localMatrix = MatrixIdentity(),
    //     .worldMatrix = MatrixIdentity(),
    //     .isDirty = true
    // });
    // ecs_set(world, node1, ModelComponent, {&cube});
    
    // // Create Entity to parent to NodeParent
    // ecs_entity_t node2 = ecs_entity(world, {
    //     .name = "NodeChild",
    //     .parent = node1
    // });
    // ecs_set(world, node2, Transform3D, {
    //     .position = (Vector3){2.0f, 0.0f, 0.0f},
    //     .rotation = QuaternionIdentity(),
    //     .scale = (Vector3){0.5f, 0.5f, 0.5f},
    //     .localMatrix = MatrixIdentity(),
    //     .worldMatrix = MatrixIdentity(),
    //     .isDirty = true
    // });
    // ecs_set(world, node2, ModelComponent, {&cube});
    
    // // Create Entity to parent to NodeParent
    // ecs_entity_t node3 = ecs_entity(world, {
    //     .name = "Node3",
    //     .parent = node1
    // });
    // ecs_set(world, node3, Transform3D, {
    //     .position = (Vector3){2.0f, 0.0f, 2.0f},
    //     .rotation = QuaternionIdentity(),
    //     .scale = (Vector3){0.5f, 0.5f, 0.5f},
    //     .localMatrix = MatrixIdentity(),
    //     .worldMatrix = MatrixIdentity(),
    //     .isDirty = true
    // });
    // ecs_set(world, node3, ModelComponent, {&cube});

    // // Create Entity to parent to NodeChild
    // ecs_entity_t node4 = ecs_entity(world, {
    //     .name = "NodeGrandchild",
    //     .parent = node2
    // });
    // ecs_set(world, node4, Transform3D, {
    //     .position = (Vector3){1.0f, 0.0f, 1.0f},
    //     .rotation = QuaternionIdentity(),
    //     .scale = (Vector3){0.5f, 0.5f, 0.5f},
    //     .localMatrix = MatrixIdentity(),
    //     .worldMatrix = MatrixIdentity(),
    //     .isDirty = true
    // });
    // ecs_set(world, node4, ModelComponent, {&cube});

    // ecs_entity_t gui = ecs_new(world);
    // ecs_set_name(world, gui, "transform_gui");  // Optional: Name for debugging
    // ecs_set(world, gui, TransformGUI, {
    //     .id = node1  // Reference the id entity
    // });

    //Loop Logic and render
    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    // UnloadModel(cube);
    ecs_fini(world);
    CloseWindow();
    return 0;
}
