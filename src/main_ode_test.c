// raylib 5.5
// flecs v4.1.1
#define RAYGUI_IMPLEMENTATION
#include <stdio.h>
#include "ecs_components.h"
// #include "module_dev.h"
// #include "module_enet.h"
#include "module_ode.h"

ecs_entity_t reset_t;
ecs_entity_t current_cube;
static ecs_query_t *cube_query = NULL;

typedef struct {
    bool reset_requested;
} ResetRequest;
ECS_COMPONENT_DECLARE(ResetRequest);

// Convert Transform3D to raylib Matrix for rendering
static Matrix transform3d_to_raylib_matrix(const Transform3D *transform) {
    // Create scale matrix
    Matrix scale_mat = MatrixScale(transform->scale.x, transform->scale.y, transform->scale.z);
    
    // Create rotation matrix
    Matrix rot_mat = QuaternionToMatrix(transform->rotation);
    
    // Create translation matrix
    Matrix trans_mat = MatrixTranslate(transform->position.x, transform->position.y, transform->position.z);
    
    // Combine: scale -> rotation -> translation (standard order)
    Matrix result = MatrixMultiply(rot_mat, scale_mat);
    result = MatrixMultiply(trans_mat, result);
    
    return result;
}

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


int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

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


    // Create an entity observer
    ecs_observer(world, {
        // Not interested in any specific component
        .query.terms = {{ EcsAny, .src.id = reset_t }},
        .events = { ecs_id(ResetRequest) },
        .callback = on_reset_cube_system
    });

    // In your module init
    // ECS_SYSTEM(world, render_transform_3d_system, RLEndMode3DPhase, Transform3D);

    // Input
    ecs_system_init(world, &(ecs_system_desc_t){
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
    ecs_set_ctx(world, &camera, NULL);

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

    // Cleanup: Add to end of main (before ecs_fini)
    // if (g_host) {
    //   enet_host_destroy(g_host);
    //   enet_deinitialize();
    // }

    ecs_fini(world);
    CloseWindow();
    return 0;
}
