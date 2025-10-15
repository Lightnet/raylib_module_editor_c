// physics 3D
// https://github.com/thomasmarsh/ODE

#include "ecs_components.h" // phase
#include "module_ode.h"
#include "raygui.h"
#include "raymath.h"


// Declare and define component in the source file
ECS_COMPONENT_DECLARE(ode_body_t);
ECS_COMPONENT_DECLARE(ode_geom_t);
ECS_COMPONENT_DECLARE(ode_context_t);


// ECS_COMPONENT_DECLARE(Transform3D);

// Helper function to convert ODE 4x4 matrix to raylib Matrix
static Matrix ode_matrix_to_raylib(const dReal *ode_matrix) {
    return (Matrix){
        .m0 = (float)ode_matrix[0],  .m4 = (float)ode_matrix[4],  .m8 = (float)ode_matrix[8],   .m12 = (float)ode_matrix[3],
        .m1 = (float)ode_matrix[1],  .m5 = (float)ode_matrix[9],  .m9 = (float)ode_matrix[12],  .m13 = (float)ode_matrix[7],
        .m2 = (float)ode_matrix[2],  .m6 = (float)ode_matrix[10], .m10 = (float)ode_matrix[13], .m14 = (float)ode_matrix[11],
        .m3 = (float)ode_matrix[12], .m7 = (float)ode_matrix[13], .m11 = (float)ode_matrix[14], .m15 = (float)ode_matrix[15]
    };
}

// Helper function to extract quaternion from 3x3 rotation matrix
static Quaternion matrix_to_quaternion(const Matrix *mat) {
    float trace = mat->m0 + mat->m5 + mat->m10;
    
    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        return (Quaternion){
            .w = 0.25f * s,
            .x = (mat->m9 - mat->m6) / s,
            .y = (mat->m2 - mat->m8) / s,
            .z = (mat->m4 - mat->m1) / s
        };
    } else if (mat->m0 > mat->m5 && mat->m0 > mat->m10) {
        float s = sqrtf(1.0f + mat->m0 - mat->m5 - mat->m10) * 2.0f;
        return (Quaternion){
            .w = (mat->m9 - mat->m6) / s,
            .x = 0.25f * s,
            .y = (mat->m1 + mat->m4) / s,
            .z = (mat->m8 + mat->m2) / s
        };
    } else if (mat->m5 > mat->m10) {
        float s = sqrtf(1.0f + mat->m5 - mat->m0 - mat->m10) * 2.0f;
        return (Quaternion){
            .w = (mat->m2 - mat->m8) / s,
            .x = (mat->m1 + mat->m4) / s,
            .y = 0.25f * s,
            .z = (mat->m6 + mat->m9) / s
        };
    } else {
        float s = sqrtf(1.0f + mat->m10 - mat->m0 - mat->m5) * 2.0f;
        return (Quaternion){
            .w = (mat->m4 - mat->m1) / s,
            .x = (mat->m8 + mat->m2) / s,
            .y = (mat->m6 + mat->m9) / s,
            .z = 0.25f * s
        };
    }
}

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

// Physics system - operates on ode_context_t component
// if ode_context_t exist start loop else it will not loop.
void ode_physics_system(ecs_iter_t *it) {
    // ode_context_t *ctx = ecs_singleton_get_mut(it->world, ode_context_t);
    // if (!ctx) return;
    ode_context_t *ctx = ecs_field(it, ode_context_t, 0);// field index 0

    // Pass ode_context_t to collision callback
    dSpaceCollide(ctx->space, ctx, &nearCallback);
    dWorldQuickStep(ctx->world, 1.0f / 60.0f);
    dJointGroupEmpty(ctx->contact_group);
}

// Sync transform system
void sync_transform_3d_system(ecs_iter_t *it){
    ode_body_t *body = ecs_field(it, ode_body_t, 0);
    Transform3D *transform = ecs_field(it, Transform3D, 1);
    
    // printf("sync\n");

    for (int i = 0; i < it->count; i++) {
        ecs_entity_t e = it->entities[i];
        const dReal *pos = dBodyGetPosition(body[i].id);
        const dReal *rot_matrix = dBodyGetRotation(body[i].id);
        
        // printf("y: %f\n", pos[1]);
        
        // Update position
        transform[i].position = (Vector3){
            .x = (float)pos[0],
            .y = (float)pos[1],
            .z = (float)pos[2]
        };
        
        // Convert ODE matrix to raylib Matrix
        Matrix ode_mat = ode_matrix_to_raylib(rot_matrix);
        
        // Extract quaternion from rotation matrix
        transform[i].rotation = matrix_to_quaternion(&ode_mat);
        
        // Normalize quaternion to avoid drift
        transform[i].rotation = QuaternionNormalize(transform[i].rotation);
        
        // Update matrices using raylib functions
        Matrix rotation_mat = QuaternionToMatrix(transform[i].rotation);
        Matrix translation_mat = MatrixTranslate(
            transform[i].position.x, 
            transform[i].position.y, 
            transform[i].position.z
        );
        
        // Combine rotation and translation
        transform[i].localMatrix = MatrixMultiply(rotation_mat, translation_mat);
        
        // Scale matrix (assuming uniform scale of 1.0)
        Matrix scale_mat = MatrixScale(1.0f, 1.0f, 1.0f);
        transform[i].localMatrix = MatrixMultiply(transform[i].localMatrix, scale_mat);
        
        // For now, assume local == world (no parent hierarchy)
        transform[i].worldMatrix = transform[i].localMatrix;
        
        // Mark as dirty
        transform[i].isDirty = true;
        
        // Notify ECS that transform changed
        // ecs_modified(it->world, e, Transform3D);
    }
}

// 
void setup_systems_ode(ecs_world_t *world){
    
    //physics
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { 
            .name = "ode_physics_system", 
            .add = ecs_ids(ecs_dependson(EcsOnUpdate)) 
        }),
        .query.terms = {
            { .id = ecs_id(ode_context_t), .src.id = ecs_id(ode_context_t) } // Singleton source
        },
      .callback = ode_physics_system
    });

    ECS_SYSTEM(world, sync_transform_3d_system, EcsPostUpdate, ode_body_t, Transform3D);

    // ecs_system_init(world, &(ecs_system_desc_t){
    //     .entity = ecs_entity(world, { 
    //         .name = "sync_transform_3d_system", 
    //         .add = ecs_ids(ecs_dependson(EcsOnUpdate)) 
    //     }),
    //     .query.terms = {
    //         { .id = ecs_id(ode_context_t) },
    //         { .id = ecs_id(Transform3D) }
    //     },
    //   .callback = sync_transform_3d_system
    // });
}


// 
void setup_components_ode(ecs_world_t *world){
    ECS_COMPONENT_DEFINE(world, ode_body_t);
    ECS_COMPONENT_DEFINE(world, ode_geom_t);
    ECS_COMPONENT_DEFINE(world, ode_context_t);
}

void module_init_ode(ecs_world_t *world){
    printf("init ode\n");
    setup_components_ode(world);

    setup_systems_ode(world);


    // Initialize ODE
    dInitODE();
    dWorldID ode_world = dWorldCreate();
    dSpaceID space = dHashSpaceCreate(0);
    dJointGroupID contact_group = dJointGroupCreate(0);
    dWorldSetGravity(ode_world, 0, -9.81f, 0);

    // Create ode_context_t singleton entity (keep as singleton for Physics system)
    ecs_singleton_set(world, ode_context_t, {
        .world = ode_world,
        .space = space,
        .contact_group = contact_group
    });


}