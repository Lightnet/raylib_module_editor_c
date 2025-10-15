#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "ode/ode.h"
#include "flecs.h"

ecs_entity_t reset_t;
ecs_entity_t current_cube;
static ecs_query_t *cube_query = NULL;

// ODE Context Component (renamed and without ground)
typedef struct {
    dWorldID world;
    dSpaceID space;
    dJointGroupID contact_group;
} ode_context_t;
ECS_COMPONENT_DECLARE(ode_context_t);

// Render Context Component
typedef struct {
    Camera3D camera;
    Model model;
} RenderContext;
ECS_COMPONENT_DECLARE(RenderContext);

typedef struct {
    bool reset_requested;
} ResetRequest;
ECS_COMPONENT_DECLARE(ResetRequest);

// Callback for collision detection
static void nearCallback(void *data, dGeomID o1, dGeomID o2) {
    ode_context_t *ctx = (ode_context_t*)data;
    if (!ctx || !ctx->world || !ctx->contact_group) return;

    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);

    // Skip if either body is null or they're already connected
    if (b1 && b2 && dAreConnected(b1, b2)) return;

    dContact contact;
    contact.surface.mode = dContactBounce;
    contact.surface.mu = dInfinity;
    contact.surface.bounce = 0.5f;
    contact.surface.bounce_vel = 0.1f;
    contact.surface.soft_cfm = 0.01f;

    // Check collision and create contact joint safely
    if (dCollide(o1, o2, 1, &contact.geom, sizeof(dContact))) {
        dJointID c = dJointCreateContact(ctx->world, ctx->contact_group, &contact);
        if (c) {
            dJointAttach(c, b1, b2);
        }
    }
}

// Components
typedef struct {
    dBodyID id;
} OdeBody;
ECS_COMPONENT_DECLARE(OdeBody);

typedef struct {
    dGeomID id;
} OdeGeom;
ECS_COMPONENT_DECLARE(OdeGeom);

typedef struct {
    float x, y, z;
} Position;
ECS_COMPONENT_DECLARE(Position);

typedef struct {
    float yaw, pitch, roll;
} YawPitchRoll;
ECS_COMPONENT_DECLARE(YawPitchRoll);

typedef struct {
    Matrix mat;
} WorldTransform;
ECS_COMPONENT_DECLARE(WorldTransform);

// Function to reset cube position randomly
void resetCubePosition(ecs_world_t *ecs_world, ecs_entity_t entity) {
    OdeBody *body = ecs_get_mut(ecs_world, entity, OdeBody);
    float x = (float)GetRandomValue(-5, 5);
    float z = (float)GetRandomValue(-5, 5);
    float y = (float)GetRandomValue(5, 15);
    dBodySetPosition(body->id, x, y, z);
    dBodySetLinearVel(body->id, 0, 0, 0);
    dBodySetAngularVel(body->id, 0, 0, 0);
    ecs_modified(ecs_world, entity, OdeBody);
}

// Convert ODE rotation matrix to yaw, pitch, roll (in degrees)
void getYawPitchRoll(const dReal *rot, float *yaw, float *pitch, float *roll) {
    float r11 = rot[0], r12 = rot[4], r13 = rot[8];
    float r21 = rot[1], r23 = rot[9];
    float r31 = rot[2], r32 = rot[6], r33 = rot[10];

    *pitch = atan2f(-r31, sqrtf(r11 * r11 + r21 * r21)) * RAD2DEG;
    *yaw = atan2f(r21, r11) * RAD2DEG;
    *roll = atan2f(r32, r33) * RAD2DEG;
}

Matrix ode_to_rl_transform(dBodyID body) {
    const dReal *pos = dBodyGetPosition(body);
    const dReal *rot = dBodyGetRotation(body);

    Matrix rot_matrix = {
        (float)rot[0], (float)rot[1], (float)rot[2],  0.0f,
        (float)rot[4], (float)rot[5], (float)rot[6],  0.0f,
        (float)rot[8], (float)rot[9], (float)rot[10], 0.0f,
        0.0f,          0.0f,          0.0f,          1.0f
    };

    Matrix trans_matrix = MatrixTranslate((float)pos[0], (float)pos[1], (float)pos[2]);
    return MatrixMultiply(rot_matrix, trans_matrix);
}

// Physics system - operates on ode_context_t component
void ode_physics_system(ecs_iter_t *it) {
    ode_context_t *ctx = ecs_singleton_get_mut(it->world, ode_context_t);

    if (!ctx) return;
    
    // Pass ode_context_t to collision callback
    dSpaceCollide(ctx->space, ctx, &nearCallback);
    dWorldQuickStep(ctx->world, 1.0f / 60.0f);
    dJointGroupEmpty(ctx->contact_group);
}

// Sync transform system
void SyncTransform(ecs_iter_t *it) {
    OdeBody *body = ecs_field(it, OdeBody, 0);
    for (int i = 0; i < it->count; i++) {
        ecs_entity_t e = it->entities[i];
        const dReal *pos = dBodyGetPosition(body[i].id);
        
        Position *p = ecs_get_mut(it->world, e, Position);
        p->x = (float)pos[0];
        p->y = (float)pos[1];
        p->z = (float)pos[2];
        ecs_modified(it->world, e, Position);

        const dReal *rot = dBodyGetRotation(body[i].id);
        YawPitchRoll *ypr = ecs_get_mut(it->world, e, YawPitchRoll);
        getYawPitchRoll(rot, &ypr->yaw, &ypr->pitch, &ypr->roll);
        ecs_modified(it->world, e, YawPitchRoll);

        WorldTransform *trans = ecs_get_mut(it->world, e, WorldTransform);
        trans->mat = ode_to_rl_transform(body[i].id);
        ecs_modified(it->world, e, WorldTransform);
    }
}

// Render system - operates on RenderContext component
// Render system - gets RenderContext from ECS context
void Render(ecs_iter_t *it) {
    const RenderContext *rctx = ecs_singleton_get(it->world, RenderContext);
    if (!rctx) return;
    
    // Query only processes entities with Position, YawPitchRoll, WorldTransform
    Position *pos = ecs_field(it, Position, 0);
    YawPitchRoll *ypr = ecs_field(it, YawPitchRoll, 1);
    WorldTransform *trans = ecs_field(it, WorldTransform, 2);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(rctx->camera);

    // Draw ground plane using Raylib's built-in function (static geom)
    DrawPlane((Vector3){0, 0, 0}, (Vector2){10, 10}, GRAY);
    
    for (int i = 0; i < it->count; i++) {
        Model temp_model = rctx->model;
        temp_model.transform = trans[i].mat;
        DrawModel(temp_model, (Vector3){0, 0, 0}, 1.0f, RED);
        DrawModelWires(temp_model, (Vector3){0, 0, 0}, 1.0f, BLACK);
        
        // Debug info for first entity only
        if (i == 0) {
            DrawText(TextFormat("Position: X: %.2f  Y: %.2f  Z: %.2f", pos[i].x, pos[i].y, pos[i].z), 10, 60, 20, DARKGRAY);
            DrawText(TextFormat("Rotation: Yaw: %.1f  Pitch: %.1f  Roll: %.1f", ypr[i].yaw, ypr[i].pitch, ypr[i].roll), 10, 90, 20, DARKGRAY);
        }
    }

    EndMode3D();

    DrawFPS(10, 10);
    DrawText("Press R to Randomize/Reset Cube", 10, 30, 20, DARKGRAY);
    EndDrawing();
}

// Reset system - runs before physics
void on_reset_cube_system(ecs_iter_t *it) {
    printf("reset!\n");
    if(current_cube != 0){
        //resetCubePosition(it->world, current_cube);

        // Use the query to iterate over all OdeBody entities
        ecs_iter_t qit = ecs_query_iter(it->world, cube_query);

        while (ecs_query_next(&qit)) {
            OdeBody *bodies = ecs_field(&qit, OdeBody, 0);
        
            for (int i = 0; i < qit.count; i++) {
                ecs_entity_t entity = qit.entities[i];
                dBodyID body = bodies[i].id;
                
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
                    ecs_modified(it->world, entity, OdeBody);
                }
            }
        }
    }
}

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


int main() {
    // Initialize Flecs
    ecs_world_t *ecs = ecs_init();

    ECS_COMPONENT_DEFINE(ecs, ode_context_t);
    ECS_COMPONENT_DEFINE(ecs, RenderContext);
    ECS_COMPONENT_DEFINE(ecs, OdeBody);
    ECS_COMPONENT_DEFINE(ecs, OdeGeom);
    ECS_COMPONENT_DEFINE(ecs, Position);
    ECS_COMPONENT_DEFINE(ecs, YawPitchRoll);
    ECS_COMPONENT_DEFINE(ecs, WorldTransform);
    ECS_COMPONENT_DEFINE(ecs, ResetRequest);

    reset_t = ecs_entity(ecs, { .name = "reset" });
    cube_query = ecs_query(ecs, {
        .terms = {{ ecs_id(OdeBody) }}
    });

    // Initialize ODE
    dInitODE();
    dWorldID world = dWorldCreate();
    dSpaceID space = dHashSpaceCreate(0);
    dJointGroupID contact_group = dJointGroupCreate(0);
    dWorldSetGravity(world, 0, -9.81f, 0);

    // Create ground as separate entity with OdeGeom
    ecs_entity_t ground_entity = ecs_entity(ecs, {.name = "Ground"});
    dGeomID ground = dCreatePlane(space, 0, 1, 0, 0); // Normal (0,1,0), distance 0
    ecs_set(ecs, ground_entity, OdeGeom, { .id = ground });

    // Create ode_context_t singleton entity (keep as singleton for Physics system)
    ecs_singleton_set(ecs, ode_context_t, {
        .world = world,
        .space = space,
        .contact_group = contact_group
    });

    // Define cube size
    const float cube_size = 1.0f;

    // Initialize Raylib
    InitWindow(800, 600, "Cube Drop Simulation (ODE with Flecs Components) - Press R to Reset");
    SetTargetFPS(60);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model cube_model = LoadModelFromMesh(GenMeshCube(cube_size, cube_size, cube_size));
    SetRandomSeed((unsigned int)GetTime());

    // Create RenderContext singleton entity
    ecs_singleton_set(ecs, RenderContext, {
        .camera = camera,
        .model = cube_model
    });

    // Create cube entity
    ecs_entity_t cube = ecs_entity(ecs, {.name = "Cube"});
    dBodyID cube_body = dBodyCreate(world);
    dMass mass;
    dMassSetBox(&mass, 1.0, cube_size, cube_size, cube_size);
    dBodySetMass(cube_body, &mass);
    dGeomID cube_geom = dCreateBox(space, cube_size, cube_size, cube_size);
    dGeomSetBody(cube_geom, cube_body);
    
    ecs_set(ecs, cube, OdeBody, { .id = cube_body });
    ecs_set(ecs, cube, OdeGeom, { .id = cube_geom });
    ecs_add(ecs, cube, Position);
    ecs_add(ecs, cube, YawPitchRoll);
    ecs_add(ecs, cube, WorldTransform);
    // ecs_add(ecs, cube, ResetRequest); // Add reset component
    ecs_set(ecs, cube, ResetRequest,{
        .reset_requested=false
    }); // Add reset component
    current_cube = cube;

    // Systems
    // ECS_SYSTEM(ecs, ResetSystem, EcsOnUpdate);
    ECS_SYSTEM(ecs, ode_physics_system, EcsOnUpdate, 0);
    ECS_SYSTEM(ecs, SyncTransform, EcsPostUpdate, OdeBody);
    ECS_SYSTEM(ecs, Render, EcsPostFrame, Position, YawPitchRoll, WorldTransform);

    // Input
    ecs_system_init(ecs, &(ecs_system_desc_t){
      .entity = ecs_entity(ecs, { .name = "test_input_system", .add = ecs_ids(ecs_dependson(EcsOnUpdate)) }),
      .callback = test_input_system
    });

    // Create an entity observer
    ecs_observer(ecs, {
        // Not interested in any specific component
        .query.terms = {{ EcsAny, .src.id = reset_t }},
        .events = { ecs_id(ResetRequest) },
        .callback = on_reset_cube_system
    });

    // Initial reset
    resetCubePosition(ecs, cube);

    // Main loop
    while (!WindowShouldClose()) {
        // if (IsKeyPressed(KEY_R)) {
        //     resetCubePosition(ecs, cube);
        //     //ecs_set(ecs, cube, ResetRequest, { .reset_requested = true });
        // }
        ecs_progress(ecs, 0);
    }

    // Cleanup
    ode_context_t *ode_ctx = ecs_singleton_get_mut(ecs, ode_context_t);
    if (ode_ctx) {
        dJointGroupDestroy(ode_ctx->contact_group);
        dSpaceDestroy(ode_ctx->space);
        dWorldDestroy(ode_ctx->world);
    }

    RenderContext *render_ctx = ecs_singleton_get_mut(ecs, RenderContext);
    if (render_ctx) {
        UnloadModel(render_ctx->model);
    }

    dCloseODE();
    CloseWindow();
    ecs_fini(ecs);

    return 0;
}