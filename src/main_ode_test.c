// raylib 5.5
// flecs v4.1.1
#define RAYGUI_IMPLEMENTATION
#include <stdio.h>
#include "ecs_components.h"
// #include "module_dev.h"
// #include "module_enet.h"
#include "module_ode.h"

// In your initialization code
typedef struct {
    Model cube_model;
} render_assets_t;

render_assets_t render_assets = {0};

void init_render_assets(float cube_size) {
    // Generate cube mesh and load as model
    render_assets.cube_model = LoadModelFromMesh(GenMeshCube(cube_size, cube_size, cube_size));
    
    // Optional: Set material properties
    // SetMaterialTexture(&render_assets.cube_model.materials[0], MAP_DIFFUSE, some_texture);
    
    printf("Cube model loaded with %d meshes\n", render_assets.cube_model.meshCount);
}

// Cleanup
void cleanup_render_assets() {
    UnloadModel(render_assets.cube_model);
}

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

// Rendering system that draws entities with Transform3D and cube model
void render_transform_3d_system(ecs_iter_t *it) {
    Transform3D *transform = ecs_field(it, Transform3D, 0);
    
    // Get delta time for smooth rendering
    // float delta_time = (float)ecs_get_target_fps(it->world) > 0 ? 
    //                    1.0f / ecs_get_target_fps(it->world) : 1.0f / 60.0f;

    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };
    DrawCube(cubePosition, 1.0f, 1.0f, 1.0f, RED);
    
    for (int i = 0; i < it->count; i++) {
        ecs_entity_t e = it->entities[i];
        
        // Skip if not dirty (optimization)
        if (!transform[i].isDirty) {
            continue;
        }
        
        // Convert transform to raylib matrix
        Matrix model_matrix = transform3d_to_raylib_matrix(&transform[i]);
        
        // Apply the transform to the model
        render_assets.cube_model.transform = model_matrix;
        
        // Draw the model at origin (transform is already applied)
        DrawModel(render_assets.cube_model, (Vector3){0, 0, 0}, 1.0f, RED);
        DrawModelWires(render_assets.cube_model, (Vector3){0, 0, 0}, 1.0f, BLACK);
        
        // Reset dirty flag after rendering
        transform[i].isDirty = false;
    }
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

int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    // Initialize components and phases
    module_init_raylib(world);
    // module_init_dev(world);
    // module_init_enet(world);
    module_init_ode(world);

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

    init_render_assets(1.0f);

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


    reset_cube_position(world, cube);



    // In your module init
    ECS_SYSTEM(world, render_transform_3d_system, RLEndMode3DPhase, Transform3D);



    

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
