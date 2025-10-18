// raylib 5.5
// flecs v4.1.1
// #define RAYGUI_IMPLEMENTATION
#include <stdio.h>
#include "ecs_components.h"
#include "module_ode.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MOUSE_SENSITIVITY 0.002f
#define MOVE_SPEED 5.0f
#define SPRINT_MULTIPLIER 2.0f
#define CAMERA_FOV 60.0f
#define CAMERA_MIN_DISTANCE 0.1f
#define MAX_PITCH 89.0f * DEG2RAD

ecs_entity_t reset_t;
ecs_entity_t current_cube;
static ecs_query_t *cube_query = NULL;

typedef struct {
    float yaw;
    float pitch;
} camera_controller_t;
ECS_COMPONENT_DECLARE(camera_controller_t);

typedef struct {
    bool reset_requested;
} ResetRequest;
ECS_COMPONENT_DECLARE(ResetRequest);

// Function to reset cube position randomly
void reset_cube_position(ecs_world_t *ecs_world, ecs_entity_t entity) {
    ode_body_t *body = ecs_get_mut(ecs_world, entity, ode_body_t);
    float x = (float)GetRandomValue(-5, 5);
    float z = (float)GetRandomValue(-5, 5);
    float y = (float)GetRandomValue(5, 15);
    dBodySetPosition(body->id, x, y, z);
    dBodySetLinearVel(body->id, 0, 0, 0);
    dBodySetAngularVel(body->id, 0, 0, 0);
    //ecs_modified(ecs_world, entity, ode_body_t);
}
// Reset system - runs before physics
void on_reset_cube_system(ecs_iter_t *it) {
    printf("reset! %d\n", current_cube);
    // if(current_cube != 0){
        //resetCubePosition(it->world, current_cube);
        
        cube_query = ecs_query(it->world, {
        .terms = {{ ecs_id(ode_body_t) }}
        });

        // Use the query to iterate over all ode_body_t entities
        ecs_iter_t qit = ecs_query_iter(it->world, cube_query);
        printf("check body: %d\n", qit.count);

        while (ecs_query_next(&qit)) {
            ode_body_t *bodies = ecs_field(&qit, ode_body_t, 0);
            
            for (int i = 0; i < qit.count; i++) {
                ecs_entity_t entity = qit.entities[i];
                dBodyID body = bodies[i].id;
                printf("found body\n");
                
                if (body) {  // Safety check
                    // Reset position and velocities
                    float x = (float)GetRandomValue(-5, 5);
                    float z = (float)GetRandomValue(-5, 5);
                    float y = (float)GetRandomValue(5, 15);
                    
                    dBodySetPosition(body, x, y, z);
                    dBodySetLinearVel(body, 0, 0, 0);
                    dBodySetAngularVel(body, 0, 0, 0);
                    
                    printf("Reset entity %lu to (%.2f, %.2f, %.2f)\n", 
                        (unsigned long)entity, x, y, z);
                    
                    // Mark component as modified for ECS
                    // ecs_modified(it->world, entity, ode_body_t);
                }
            }
        }

        ecs_query_fini(cube_query);
    // }
}
// user input
void test_input_system(ecs_iter_t *it){
    // printf("hello wolrd!\n");
    if (IsKeyPressed(KEY_R)) {
        printf("hello wolrd!\n");
        // Emit entity event.
        ecs_emit(it->world, &(ecs_event_desc_t) {
            .event = ecs_id(ResetRequest),
            .entity = reset_t,
            .param = &(ResetRequest){.reset_requested=true}
        });
    }
}
// draw raylib grid
void render_3d_grid(ecs_iter_t *it){
    DrawGrid(10, 1.0f);
}

// spector camera 3d
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

void render_2d_info_system(ecs_iter_t *it){
    DrawText("R Key to reset Cube.", 0, 30, 20, BLACK );
}

// main
int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    ECS_COMPONENT_DEFINE(world, camera_controller_t);
    ECS_COMPONENT_DEFINE(world, ResetRequest);

    reset_t = ecs_entity(world, { .name = "reset" });
    cube_query = ecs_query(world, {
        .terms = {{ ecs_id(ode_body_t) }}
    });

    // Initialize components and phases
    module_init_raylib(world);
    // module_init_dev(world);
    // module_init_enet(world);
    module_init_ode(world);

    ECS_SYSTEM(world, render_3d_grid, RLRender3DPhase);
    ECS_SYSTEM(world, render_2d_info_system, RLRender2D1Phase);

    // Create an entity observer
    ecs_observer(world, {
        // Not interested in any specific component
        .query.terms = {{ EcsAny, .src.id = reset_t }},
        .events = { ecs_id(ResetRequest) },
        .callback = on_reset_cube_system
    });

    // Input for camera
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "camera_input_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) }, // Singleton
            { .id = ecs_id(camera_controller_t), .src.id = ecs_id(camera_controller_t) }   // Singleton
        },
        .callback = camera_input_system
    });

    // Input
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "test_input_system", .add = ecs_ids(ecs_dependson(EcsOnUpdate)) }),
      .callback = test_input_system
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

    const ode_context_t *ode_context = ecs_singleton_get_mut(world, ode_context_t);


    // Create ground as separate entity with OdeGeom
    ecs_entity_t ground_entity = ecs_entity(world, {.name = "Ground"});
    dGeomID ground = dCreatePlane(ode_context->space, 0, 1, 0, 0); // Normal (0,1,0), distance 0
    ecs_set(world, ground_entity, ode_geom_t, { .id = ground });

    // init_render_assets(1.0f);

    // Create cube entity
    // Define cube size
    const float cube_size = 1.0f;
    ecs_entity_t cube = ecs_entity(world, {.name = "Cube"});
    
    dBodyID cube_body = dBodyCreate(ode_context->world);
    dMass mass;
    dMassSetBox(&mass, 1.0, cube_size, cube_size, cube_size);
    dBodySetMass(cube_body, &mass);
    dGeomID cube_geom = dCreateBox(ode_context->space, cube_size, cube_size, cube_size);
    dGeomSetBody(cube_geom, cube_body);

    ecs_set(world, cube, ode_body_t, { .id = cube_body });
    ecs_set(world, cube, ode_geom_t, { .id = cube_geom });
    ecs_set(world, cube, Transform3D, {
      .position = (Vector3){0.0f, 0.0f, 0.0f},
      .rotation = QuaternionIdentity(),
      .scale = (Vector3){1.0f, 1.0f, 1.0f},
      .localMatrix = MatrixIdentity(),
      .worldMatrix = MatrixIdentity(),
      .isDirty = true
    });
    // create Model
    Model cuber = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    ecs_set(world, cube, ModelComponent, {&cuber});
    ecs_set(world, cube, ResetRequest,{
        .reset_requested=false
    }); // Add reset component
    current_cube = cube;
    reset_cube_position(world, cube);

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
    ecs_set(world, node1, ModelComponent, {&cuber});


    //Loop Logic and render
    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    //clean up
    ecs_fini(world);
    CloseWindow();
    return 0;
}

