// ecs_components.c
// raylib set up.
// 

#include "ecs_components.h"
#include <stdio.h>
// #define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Define global phase entities
ecs_entity_t PreLogicUpdatePhase = 0;
ecs_entity_t LogicUpdatePhase = 0;
ecs_entity_t RLBeginDrawingPhase = 0;
ecs_entity_t RLRender2D0Phase = 0;
ecs_entity_t RLBeginMode3DPhase = 0;
ecs_entity_t RLRender3DPhase = 0;
ecs_entity_t RLEndMode3DPhase = 0;
ecs_entity_t RLRender2D1Phase = 0;
ecs_entity_t RLEndDrawingPhase = 0;

ECS_COMPONENT_DECLARE(main_context_t);
ECS_COMPONENT_DECLARE(Transform3D);
ECS_COMPONENT_DECLARE(ModelComponent);

// Helper function to update a single transform
void UpdateTransform(ecs_world_t *world, ecs_entity_t entity, Transform3D *transform) {
  // Get parent entity
  ecs_entity_t parent = ecs_get_parent(world, entity);
  const char *name = ecs_get_name(world, entity) ? ecs_get_name(world, entity) : "(unnamed)";
  bool parentIsDirty = false;

  // Check if parent is dirty
  if (parent && ecs_is_valid(world, parent)) {
      const Transform3D *parent_transform = ecs_get(world, parent, Transform3D);
      if (parent_transform && parent_transform->isDirty) {
          parentIsDirty = true;
      }
  }

  // Skip update if neither this transform nor its parent is dirty
  if (!transform->isDirty && !parentIsDirty) {
    //   printf("Skipping update for %s (not dirty)\n", name);
      return;
  }
//   printf("( dirty)\n", name);

  // Compute local transform
  Matrix translation = MatrixTranslate(transform->position.x, transform->position.y, transform->position.z);
  Matrix rotation = QuaternionToMatrix(transform->rotation);
  Matrix scaling = MatrixScale(transform->scale.x, transform->scale.y, transform->scale.z);
  transform->localMatrix = MatrixMultiply(scaling, MatrixMultiply(rotation, translation));

  if (!parent || !ecs_is_valid(world, parent)) {
      // Root entity: world matrix = local matrix
      transform->worldMatrix = transform->localMatrix;
    //   printf("Root %s position (%.2f, %.2f, %.2f)\n", name, transform->position.x, transform->position.y, transform->position.z);
  } else {
      // Child entity: world matrix = local matrix * parent world matrix
      const Transform3D *parent_transform = ecs_get(world, parent, Transform3D);
      if (!parent_transform) {
        //   printf("Error: Parent %s lacks Transform3D for %s\n",
        //          ecs_get_name(world, parent) ? ecs_get_name(world, parent) : "(unnamed)", name);
          transform->worldMatrix = transform->localMatrix;
          return;
      }

      // Validate parent world matrix
      float px = parent_transform->worldMatrix.m12;
      float py = parent_transform->worldMatrix.m13;
      float pz = parent_transform->worldMatrix.m14;
      if (fabs(px) > 1e6 || fabs(py) > 1e6 || fabs(pz) > 1e6) {
        //   printf("Error: Invalid parent %s world pos (%.2f, %.2f, %.2f) for %s\n",
        //          ecs_get_name(world, parent) ? ecs_get_name(world, parent) : "(unnamed)",
        //          px, py, pz, name);
          transform->worldMatrix = transform->localMatrix;
          return;
      }

      // Compute world matrix
      transform->worldMatrix = MatrixMultiply(transform->localMatrix, parent_transform->worldMatrix);

      // Extract world position
      float wx = transform->worldMatrix.m12;
      float wy = transform->worldMatrix.m13;
      float wz = transform->worldMatrix.m14;

      // Debug output
    //   const char *parent_name = ecs_get_name(world, parent) ? ecs_get_name(world, parent) : "(unnamed)";
    //   printf("Child %s (ID: %llu), parent %s (ID: %llu)\n",
    //          name, (unsigned long long)entity, parent_name, (unsigned long long)parent);
    //   printf("Child %s position (%.2f, %.2f, %.2f), parent %s world pos (%.2f, %.2f, %.2f), world pos (%.2f, %.2f, %.2f)\n",
    //          name, transform->position.x, transform->position.y, transform->position.z,
    //          parent_name, px, py, pz, wx, wy, wz);
  }

  // Mark children as dirty to ensure they update in the next frame
  ecs_iter_t it = ecs_children(world, entity);
  while (ecs_children_next(&it)) {
      for (int i = 0; i < it.count; i++) {
          Transform3D *child_transform = ecs_get_mut(world, it.entities[i], Transform3D);
          if (child_transform) {
              child_transform->isDirty = true;
              //ecs_set(world, it.entities[i], Transform3D, *child_transform);
          }
      }
  }

  // Reset isDirty after updating
  transform->isDirty = false;
}

// Function to update a single entity and its descendants
void UpdateChildTransform(ecs_world_t *world, ecs_entity_t entity) {
    Transform3D *transform = ecs_get_mut(world, entity, Transform3D);
    if (!transform) return;

    // Update the entity's transform
    UpdateTransform(world, entity, transform);
    // ecs_modified(world, entity, Transform3D);

    // Recursively update descendants
    ecs_iter_t it = ecs_children(world, entity);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            UpdateChildTransform(world, it.entities[i]);
        }
    }
}

// System to process Transform3D entities
void update_transform_3d_system(ecs_iter_t *it) {
    // Transform3D *guis = ecs_field(it, Transform3D, 0);
    for (int i = 0; i < it->count; i++) {
        ecs_entity_t entity = it->entities[i];
        if (ecs_is_valid(it->world, entity) && ecs_has(it->world, entity, Transform3D)) {
            UpdateChildTransform(it->world, entity);
        }
    }
}

// Render begin system
void RLRenderDrawingSystem(ecs_iter_t *it) {
  // printf("RenderBeginSystem\n");
  BeginDrawing();
  ClearBackground(RAYWHITE);
}

// Render begin system
void RLBeginMode3DSystem(ecs_iter_t *it) {
    // printf("BeginCamera3DSystem\n");
    main_context_t *main_context = ecs_field(it, main_context_t, 0);
    if(!main_context) return;
    BeginMode3D(main_context->camera);
}

// Camera3d system for 3d model
void RLRender3DSystem(ecs_iter_t *it) {
  Transform3D *t = ecs_field(it, Transform3D, 0);
  ModelComponent *m = ecs_field(it, ModelComponent, 1);

  for (int i = 0; i < it->count; i++) {
      ecs_entity_t entity = it->entities[i];
      if (!ecs_is_valid(it->world, entity)) {
          //printf("Skipping invalid entity ID: %llu\n", (unsigned long long)entity);
          continue;
      }
      const char *name = ecs_get_name(it->world, entity) ? ecs_get_name(it->world, entity) : "(unnamed)";
      if (!ecs_has(it->world, entity, Transform3D) || !ecs_has(it->world, entity, ModelComponent)) {
          //printf("Skipping entity %s: missing Transform3D or ModelComponent\n", name);
          continue;
      }
      // Check for garbage values in world matrix
      if (fabs(t[i].worldMatrix.m12) > 1e6 || fabs(t[i].worldMatrix.m13) > 1e6 || fabs(t[i].worldMatrix.m14) > 1e6) {
          // printf("Skipping entity %s: invalid world matrix (%.2f, %.2f, %.2f)\n",
          //        name, t[i].worldMatrix.m12, t[i].worldMatrix.m13, t[i].worldMatrix.m14);
          continue;
      }
      // printf("Rendering entity %s at world pos (%.2f, %.2f, %.2f)\n",
      //        name, t[i].worldMatrix.m12, t[i].worldMatrix.m13, t[i].worldMatrix.m14);
      if (!m[i].model) {
          // printf("Skipping entity %s: null model\n", name);
          continue;
      }
      m[i].model->transform = t[i].worldMatrix;
      bool isChild = ecs_has_pair(it->world, entity, EcsChildOf, EcsWildcard);
      DrawModel(*(m[i].model), (Vector3){0, 0, 0}, 1.0f, isChild ? BLUE : RED);
  }
//   DrawGrid(10, 1.0f);

  // does not work here.
  // Camera3D *camera = (Camera3D *)ecs_get_ctx(it->world);
  // if (camera) {
  //     DrawText(TextFormat("Camera Pos: %.2f, %.2f, %.2f",
  //                         camera->position.x, camera->position.y, camera->position.z), 10, 90, 20, DARKGRAY);
  //     DrawText(TextFormat("Entities Rendered: %d", it->count), 10, 110, 20, DARKGRAY);
  // }
}

// RL EndMode3D
void RLEndMode3DSystem(ecs_iter_t *it) {
    main_context_t *main_context = ecs_field(it, main_context_t, 0);
    if (!main_context) return;
    EndMode3D();
}

//  End Drawing system
void RLEndDrawingSystem(ecs_iter_t *it) {
  //printf("EndRenderSystem\n");
  EndDrawing();
}

// setup phase for pipeline loop
void setup_phases(ecs_world_t *world){
    // Define custom phases
    // ecs_entity_t PreLogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
    // ecs_entity_t LogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
    // ecs_entity_t RLBeginDrawingPhase = ecs_new_w_id(world, EcsPhase);
    // ecs_entity_t RLRender2D0Phase = ecs_new_w_id(world, EcsPhase);
    // ecs_entity_t RLBeginMode3DPhase = ecs_new_w_id(world, EcsPhase);
    // ecs_entity_t RLRender3DPhase = ecs_new_w_id(world, EcsPhase);
    // ecs_entity_t RLEndMode3DPhase = ecs_new_w_id(world, EcsPhase);
    // ecs_entity_t RLRender2D1Phase = ecs_new_w_id(world, EcsPhase);
    // ecs_entity_t RLEndDrawingPhase = ecs_new_w_id(world, EcsPhase);

    PreLogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
    LogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
    RLBeginDrawingPhase = ecs_new_w_id(world, EcsPhase);
    RLRender2D0Phase = ecs_new_w_id(world, EcsPhase);
    RLBeginMode3DPhase = ecs_new_w_id(world, EcsPhase);
    RLRender3DPhase = ecs_new_w_id(world, EcsPhase);
    RLEndMode3DPhase = ecs_new_w_id(world, EcsPhase);
    RLRender2D1Phase = ecs_new_w_id(world, EcsPhase);
    RLEndDrawingPhase = ecs_new_w_id(world, EcsPhase);

    // Order phases
    ecs_add_pair(world, PreLogicUpdatePhase, EcsDependsOn, EcsPreUpdate);
    ecs_add_pair(world, LogicUpdatePhase, EcsDependsOn, PreLogicUpdatePhase);
    ecs_add_pair(world, RLBeginDrawingPhase, EcsDependsOn, LogicUpdatePhase);
    ecs_add_pair(world, RLRender2D0Phase, EcsDependsOn, LogicUpdatePhase);
    ecs_add_pair(world, RLBeginMode3DPhase, EcsDependsOn, RLRender2D0Phase);
    ecs_add_pair(world, RLRender3DPhase, EcsDependsOn, RLBeginMode3DPhase);
    ecs_add_pair(world, RLEndMode3DPhase, EcsDependsOn, RLRender3DPhase);
    ecs_add_pair(world, RLRender2D1Phase, EcsDependsOn, RLEndMode3DPhase);
    ecs_add_pair(world, RLEndDrawingPhase, EcsDependsOn, RLRender2D1Phase);
}

void setup_components(ecs_world_t *world){
    ECS_COMPONENT_DEFINE(world, Transform3D);
    ECS_COMPONENT_DEFINE(world, ModelComponent);
    ECS_COMPONENT_DEFINE(world, main_context_t);

}

void setup_systems(ecs_world_t *world){

    // transform 3d Hierarchy update
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "update_transform_3d_system",
            .add = ecs_ids(ecs_dependson(PreLogicUpdatePhase))
        }),
        .query.terms = {
            // { .id = ecs_id(TransformGUI), .src.id = EcsSelf }
            { .id = ecs_id(Transform3D), .src.id = EcsSelf }
        },
        .callback = update_transform_3d_system
    });

    // note this has be in order of the ECS since push into array.
    // Render Begin System
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "RenderBeginDrawingSystem", 
        .add = ecs_ids(ecs_dependson(RLBeginDrawingPhase)) 
      }),
      .callback = RLRenderDrawingSystem
    });

    // Begin Mode 3D camera System
    ecs_system(world, {
      .entity = ecs_entity(world, {
        .name = "BeginMode3DSystem", 
        .add = ecs_ids(ecs_dependson(RLBeginMode3DPhase))
      }),
      .query.terms = {
        { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) } 
      },
      .callback = RLBeginMode3DSystem
    });

    // Camera 3D System
    ecs_system(world,{
        .entity = ecs_entity(world, {
          .name = "RLRender3DSystem", 
          .add = ecs_ids(ecs_dependson(RLRender3DPhase))
        }),
        .query.terms = {
            { .id = ecs_id(Transform3D), .src.id = EcsSelf },
            { .id = ecs_id(ModelComponent), .src.id = EcsSelf }
        },
        .callback = RLRender3DSystem
    });

    // End Camera 3D System
    ecs_system(world, {
      .entity = ecs_entity(world, {
        .name = "RLEndMode3DSystem", 
        .add = ecs_ids(ecs_dependson(RLEndMode3DPhase))
      }),
      .query.terms = {
        { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) } 
      },
      .callback = RLEndMode3DSystem
    });

    // End Render gl System
    ecs_system(world, {
        .entity = ecs_entity(world, {
          .name = "EndDrawingSystem", 
          .add = ecs_ids(ecs_dependson(RLEndDrawingPhase)) 
        }),
        .callback = RLEndDrawingSystem
    });

}

// Initialize Raylib-related components and phases
void module_init_raylib(ecs_world_t *world) {
    //phases
    setup_phases(world);
    // Define components
    setup_components(world);
    //systems
    setup_systems(world);
}
