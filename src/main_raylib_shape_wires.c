// raylib 5.5
// flecs v4.1.1

// https://www.raylib.com/cheatsheet/cheatsheet.html

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

// void DrawCubeWires(Vector3 position, float width, float height, float length, Color color);        // Draw cube wires
typedef struct {
    Vector3 position;
    float width;
    float height;
    float length;
    Color color;
} cube_wire_t;
ECS_COMPONENT_DECLARE(cube_wire_t);

// void DrawCapsule(Vector3 startPos, Vector3 endPos, float radius, int slices, int rings, Color color); // Draw a capsule with the center of its sphere caps at startPos and endPos
typedef struct {
    Vector3 position;
    Vector3 start_pos;
    Vector3 end_pos;
    float radius;
    float height;
    int slices;
    int rings;
    Color color;
} capsule_wire_t;
ECS_COMPONENT_DECLARE(capsule_wire_t);

// void DrawSphereWires(Vector3 centerPos, float radius, int rings, int slices, Color color);         // Draw sphere wires
typedef struct {
    Vector3 center_pos;
    float radius;
    int slices;
    int rings;
    Color color;
} sphere_wire_t;
ECS_COMPONENT_DECLARE(sphere_wire_t);

// void DrawCylinderWires(Vector3 position, float radiusTop, float radiusBottom, float height, int slices, Color color); // Draw a cylinder/cone wires
typedef struct {
    Vector3 position;
    float radius_top;
    float radius_bottom;
    float height; 
    int slices;
    Color color;
} cylinder_wire_t;
ECS_COMPONENT_DECLARE(cylinder_wire_t);

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

// render 3d draw capsule wires
void render_3d_draw_capsule_wires(ecs_iter_t *it){
    capsule_wire_t *capsule_wire = ecs_field(it, capsule_wire_t, 0);
    // Iterate matched entities
    for (int i = 0; i < it->count; i++) {
        DrawCapsuleWires(capsule_wire[i].start_pos, 
                        capsule_wire[i].end_pos,
                        capsule_wire[i].radius,
                        capsule_wire[i].slices,
                        capsule_wire[i].rings,
                        capsule_wire[i].color);
    }
}

// render 3d draw sphere wires
void render_3d_draw_sphere_wires(ecs_iter_t *it){
    sphere_wire_t *sphere_wire = ecs_field(it, sphere_wire_t, 0);
    // Iterate matched entities
    for (int i = 0; i < it->count; i++) {
        DrawSphereWires(sphere_wire[i].center_pos, 
                        sphere_wire[i].radius,
                        sphere_wire[i].slices,
                        sphere_wire[i].rings,
                        sphere_wire[i].color);
    }
}

// render 3d draw cylinder wires
void render_3d_draw_cylinder_wires(ecs_iter_t *it){
    cylinder_wire_t *cylinder_wire = ecs_field(it, cylinder_wire_t, 0);
    // Iterate matched entities
    for (int i = 0; i < it->count; i++) {
        DrawCylinderWires(cylinder_wire[i].position, 
                        cylinder_wire[i].radius_top,
                        cylinder_wire[i].radius_bottom,
                        cylinder_wire[i].height,
                        cylinder_wire[i].slices,
                        cylinder_wire[i].color);
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

// draw crosshair
void render_2d_draw_cross_point(ecs_iter_t *it){
    DrawCircleLines((int)(WINDOW_WIDTH/2), (int)(WINDOW_HEIGHT/2), 8, DARKBLUE);
}

// main
int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    ECS_COMPONENT_DEFINE(world, camera_controller_t);
    ECS_COMPONENT_DEFINE(world, cube_wire_t);
    ECS_COMPONENT_DEFINE(world, capsule_wire_t);
    ECS_COMPONENT_DEFINE(world, sphere_wire_t);
    ECS_COMPONENT_DEFINE(world, cylinder_wire_t);

    // Initialize components and phases
    module_init_raylib(world);
    module_init_dev(world);

    ECS_SYSTEM(world, render_3d_grid, RLRender3DPhase);

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
        },
        .callback = render_3d_draw_cube_wires
    });

    // Render 3D Capsule
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_3d_draw_capsule_wires", .add = ecs_ids(ecs_dependson(RLRender3DPhase)) }),
        .query.terms = {
            { .id = ecs_id(capsule_wire_t) }, //
        },
        .callback = render_3d_draw_capsule_wires
    });

    // Render 3D Sphere
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_3d_draw_sphere_wires", .add = ecs_ids(ecs_dependson(RLRender3DPhase)) }),
        .query.terms = {
            { .id = ecs_id(sphere_wire_t) }, //
        },
        .callback = render_3d_draw_sphere_wires
    });

    // Render 3D Cylinder
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_3d_draw_cylinder_wires", .add = ecs_ids(ecs_dependson(RLRender3DPhase)) }),
        .query.terms = {
            { .id = ecs_id(cylinder_wire_t) }, //
        },
        .callback = render_3d_draw_cylinder_wires
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

    // draw center crosshair
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

    ecs_entity_t capsule_wired_1 = ecs_entity(world, {
      .name = "Capsule"
    });
    ecs_set(world, capsule_wired_1, capsule_wire_t, {
        .position = (Vector3) {0,2,0},
        .start_pos = (Vector3) {0,0,0},
        .end_pos = (Vector3) {0,2,0},
        .radius = 1.0f,
        .height = 2.0f,
        .slices = 8,
        .rings = 8,
        .color = BLACK
    });

    ecs_entity_t sphere_wired_1 = ecs_entity(world, {
      .name = "Sphere1"
    });
    ecs_set(world, sphere_wired_1, sphere_wire_t, {
        .center_pos = (Vector3) {2,2,0},
        .radius = 2.0f,
        .slices = 8,
        .rings = 8,
        .color = YELLOW
    });

    ecs_entity_t cylinder_wired_1 = ecs_entity(world, {
      .name = "cylinder"
    });
    ecs_set(world, cylinder_wired_1, cylinder_wire_t, {
        .position = (Vector3) {2,2,0},
        .radius_top = 2.0f,
        .radius_bottom = 2.0f,
        .height = 2.0f,
        .slices = 8,
        .color = ORANGE
    });

    //Loop Logic and render
    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    // UnloadModel(cube);
    ecs_fini(world);
    CloseWindow();
    return 0;
}
