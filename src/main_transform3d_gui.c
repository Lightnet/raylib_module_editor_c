// raylib 5.5
// flecs v4.1.1
// strange rotation for gui
// y -85 to 85 is cap else it will have strange rotate.
// note that child does not update the changes. which need parent to update.
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "flecs.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Transform3D component
typedef struct {
    Vector3 position;         // Local position
    Quaternion rotation;      // Local rotation
    Vector3 scale;            // Local scale
    Matrix localMatrix;       // Local transform matrix
    Matrix worldMatrix;       // World transform matrix
    bool isDirty;             // Flag to indicate if transform needs updating
} Transform3D;
ECS_COMPONENT_DECLARE(Transform3D);

// Pointer component for raylib Model
typedef struct {
    Model* model;             // Pointer to Model
} ModelComponent;
ECS_COMPONENT_DECLARE(ModelComponent);

typedef struct {
  bool isMovementMode;
  bool tabPressed;
  bool moveForward;
  bool moveBackward;
  bool moveLeft;
  bool moveRight;
} PlayerInput_T;
ECS_COMPONENT_DECLARE(PlayerInput_T);

typedef struct {
    ecs_entity_t id;  // Entity to edit (e.g., cube with CubeWire)
    int selectedIndex; // Index of the selected entity in the list
    // Later: Add fields for other GUI controls
} TransformGUI;
ECS_COMPONENT_DECLARE(TransformGUI);

// place holder debug
void GUI_Transform3D_System(ecs_iter_t *it);
void GUI_Transform3D_List_System(ecs_iter_t *it);
void PickingSystem(ecs_iter_t *it);

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
      // printf("Skipping update for %s (not dirty)\n", name);
      return;
  }

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

// Recursive function to process entity and its children
void ProcessEntityHierarchy(ecs_world_t *world, ecs_entity_t entity) {
  // Update the current entity's transform
  Transform3D *transform = ecs_get_mut(world, entity, Transform3D);
  bool wasUpdated = false;
  if (transform) {
      // Only update if dirty or parent is dirty
      ecs_entity_t parent = ecs_get_parent(world, entity);
      bool parentIsDirty = false;
      if (parent && ecs_is_valid(world, parent)) {
          const Transform3D *parent_transform = ecs_get(world, parent, Transform3D);
          if (parent_transform && parent_transform->isDirty) {
              parentIsDirty = true;
          }
      }
      if (transform->isDirty || parentIsDirty) {
          UpdateTransform(world, entity, transform);
          //ecs_set(world, entity, Transform3D, *transform); // Commit changes
          wasUpdated = true;
      }
  }

  // Skip processing children if this entity was not updated
  if (!wasUpdated) {
      return;
  }

  // Iterate through children
  ecs_iter_t it = ecs_children(world, entity);
  while (ecs_children_next(&it)) {
      for (int i = 0; i < it.count; i++) {
          ecs_entity_t child = it.entities[i];
          ProcessEntityHierarchy(world, child); // Recursively process child
      }
  }
}

// System to update all transforms in hierarchical order
void UpdateTransformHierarchySystem(ecs_iter_t *it) {
  // Process only root entities (no parent)
  Transform3D *transforms = ecs_field(it, Transform3D, 0);
  for (int i = 0; i < it->count; i++) {
      ecs_entity_t entity = it->entities[i];
      ProcessEntityHierarchy(it->world, entity);
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
  Camera3D *camera = (Camera3D *)ecs_get_ctx(it->world);
  if (!camera) return;
  BeginMode3D(*camera);
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
  DrawGrid(10, 1.0f);

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
  //printf("RLEndMode3DSystem\n");
  Camera3D *camera = (Camera3D *)ecs_get_ctx(it->world);
  if (!camera) return;
  EndMode3D();
}

//  End Drawing system
void RLEndDrawingSystem(ecs_iter_t *it) {
  //printf("EndRenderSystem\n");
  EndDrawing();
}

// Input handling system
void user_input_system(ecs_iter_t *it) {
    PlayerInput_T *pi_ctx = ecs_singleton_ensure(it->world, PlayerInput_T);
    if (!pi_ctx) return;

    Transform3D *t = ecs_field(it, Transform3D, 0);
    float dt = GetFrameTime();
    //test user input
    for (int i = 0; i < it->count; i++) {
      const char *name = ecs_get_name(it->world, it->entities[i]);
      if (name) {
        bool isFound = false;
        if (strcmp(name, "NodeParent") == 0) {

          bool wasModified = false;

          if (IsKeyPressed(KEY_TAB)) {
            pi_ctx->isMovementMode = !pi_ctx->isMovementMode;
            // printf("Toggled mode to: %s\n", pi_ctx->isMovementMode ? "Movement" : "Rotation");
          }

          if (pi_ctx->isMovementMode) {
              if (IsKeyDown(KEY_W)){t[i].position.z -= 2.0f * dt;wasModified = true;}
              if (IsKeyDown(KEY_S)){t[i].position.z += 2.0f * dt;wasModified = true;}
              if (IsKeyDown(KEY_A)){t[i].position.x -= 2.0f * dt;wasModified = true;}
              if (IsKeyDown(KEY_D)){t[i].position.x += 2.0f * dt;wasModified = true;}
          } else {
              float rotateSpeed = 90.0f;
              if (IsKeyDown(KEY_Q)) {
                  Quaternion rot = QuaternionFromAxisAngle((Vector3){0, 1, 0}, DEG2RAD * rotateSpeed * dt);
                  t[i].rotation = QuaternionMultiply(t[i].rotation, rot);
                  wasModified = true;
              }
              if (IsKeyDown(KEY_E)) {
                  Quaternion rot = QuaternionFromAxisAngle((Vector3){0, 1, 0}, -DEG2RAD * rotateSpeed * dt);
                  t[i].rotation = QuaternionMultiply(t[i].rotation, rot);
                  wasModified = true;
              }
              if (IsKeyDown(KEY_W)) {
                  Quaternion rot = QuaternionFromAxisAngle((Vector3){1, 0, 0}, DEG2RAD * rotateSpeed * dt);
                  t[i].rotation = QuaternionMultiply(t[i].rotation, rot);
                  wasModified = true;
              }
              if (IsKeyDown(KEY_S)) {
                  Quaternion rot = QuaternionFromAxisAngle((Vector3){1, 0, 0}, -DEG2RAD * rotateSpeed * dt);
                  t[i].rotation = QuaternionMultiply(t[i].rotation, rot);
                  wasModified = true;
              }
              if (IsKeyDown(KEY_A)) {
                  Quaternion rot = QuaternionFromAxisAngle((Vector3){0, 0, 1}, DEG2RAD * rotateSpeed * dt);
                  t[i].rotation = QuaternionMultiply(t[i].rotation, rot);
                  wasModified = true;
              }
              if (IsKeyDown(KEY_D)) {
                  Quaternion rot = QuaternionFromAxisAngle((Vector3){0, 0, 1}, -DEG2RAD * rotateSpeed * dt);
                  t[i].rotation = QuaternionMultiply(t[i].rotation, rot);
                  wasModified = true;
              }
          }
          if (IsKeyPressed(KEY_R)) {
              t[i].position = (Vector3){0.0f, 0.0f, 0.0f};
              t[i].rotation = QuaternionIdentity();
              t[i].scale = (Vector3){1.0f, 1.0f, 1.0f};
              wasModified = true;
          }
          if (wasModified) {
            t[i].isDirty = true;
            // printf("Marked %s as dirty\n", name);// this will update if move main node
          }
        }
      }
    }
}

// render 2d only
void render2d_hud_system(ecs_iter_t *it){
  PlayerInput_T *pi_ctx = ecs_singleton_ensure(it->world, PlayerInput_T);
  if (!pi_ctx) return;
  Transform3D *t = ecs_field(it, Transform3D, 0);
  float dt = GetFrameTime();

  for (int i = 0; i < it->count; i++) {
    DrawText(TextFormat("Entity %s Pos: %.2f, %.2f, %.2f", 
      ecs_get_name(it->world, it->entities[i]) ? ecs_get_name(it->world, it->entities[i]) : "unnamed", 
      t[i].position.x, t[i].position.y, t[i].position.z), 10, 130 + i * 20, 20, DARKGRAY);
  }
  DrawText(TextFormat("Entity Count: %d", it->count), 10, 10, 20, DARKGRAY);
  DrawText(pi_ctx->isMovementMode ? "Mode: Movement (WASD)" : "Mode: Rotation (QWE/ASD)", 10, 30, 20, DARKGRAY);
  DrawText("Tab: Toggle Mode | R: Reset", 10, 50, 20, DARKGRAY);
  DrawFPS(10, 70);
}

int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    ECS_COMPONENT_DEFINE(world, Transform3D);
    ECS_COMPONENT_DEFINE(world, ModelComponent);
    ECS_COMPONENT_DEFINE(world, PlayerInput_T);
    ECS_COMPONENT_DEFINE(world, TransformGUI);

    // Define custom phases
    ecs_entity_t PreLogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t LogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t RLBeginDrawingPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t RLRender2D0Phase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t RLBeginMode3DPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t RLRender3DPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t RLEndMode3DPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t RLRender2D1Phase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t RLEndDrawingPhase = ecs_new_w_id(world, EcsPhase);

    // order phase must be in order
    ecs_add_pair(world, PreLogicUpdatePhase, EcsDependsOn, EcsPreUpdate); // start game logics
    ecs_add_pair(world, LogicUpdatePhase, EcsDependsOn, PreLogicUpdatePhase); // start game logics
    ecs_add_pair(world, RLBeginDrawingPhase, EcsDependsOn, LogicUpdatePhase); // BeginDrawing
    ecs_add_pair(world, RLRender2D0Phase, EcsDependsOn, LogicUpdatePhase); // Render 2D 0
    ecs_add_pair(world, RLBeginMode3DPhase, EcsDependsOn, RLRender2D0Phase); // EcsOnUpdate, BeginMode3D
    ecs_add_pair(world, RLRender3DPhase, EcsDependsOn, RLBeginMode3DPhase); // 3d model only
    ecs_add_pair(world, RLEndMode3DPhase, EcsDependsOn, RLRender3DPhase); // End Camera and EndMode3D
    ecs_add_pair(world, RLRender2D1Phase, EcsDependsOn, RLEndMode3DPhase); // render 2D only
    ecs_add_pair(world, RLEndDrawingPhase, EcsDependsOn, RLRender2D1Phase); // End render to screen

    // INPUT Current Entity transform3d
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "user_input_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(Transform3D), .src.id = EcsSelf },
        },
        .callback = user_input_system
    });

    // ONLY 2D
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "render2d_hud_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
      .query.terms = {
          { .id = ecs_id(Transform3D), .src.id = EcsSelf },
          { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot }
      },
      .callback = render2d_hud_system
    });

    // Transform Hierarchy
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, {
          .name = "UpdateTransformHierarchySystem",
          .add = ecs_ids(ecs_dependson(PreLogicUpdatePhase))
      }),
      .query.terms = {
          { .id = ecs_id(Transform3D), .src.id = EcsSelf },
          { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot } // No parent
      },
      .callback = UpdateTransformHierarchySystem
    });

    // note this has be in order of the ECS since push into array.
    // Render Begin System
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "RenderBeginDrawingSystem", 
        .add = ecs_ids(ecs_dependson(RLBeginDrawingPhase)) 
      }),
      .callback = RLRenderDrawingSystem
    });

    // Begin Mode 3D camera System
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, {
        .name = "BeginMode3DSystem", 
        .add = ecs_ids(ecs_dependson(RLBeginMode3DPhase)) 
      }),
      .callback = RLBeginMode3DSystem
    });

    // Camera 3D System
    ecs_system_init(world, &(ecs_system_desc_t){
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
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, {
        .name = "RLEndMode3DSystem", 
        .add = ecs_ids(ecs_dependson(RLEndMode3DPhase))
      }),
      .callback = RLEndMode3DSystem
    });

    // End Render gl System
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, {
          .name = "EndDrawingSystem", 
          .add = ecs_ids(ecs_dependson(RLEndDrawingPhase)) 
        }),
        .callback = RLEndDrawingSystem
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
    // printf("Node1 entity ID: %llu (%s)\n", (unsigned long long)node1, ecs_get_name(world, node1));
    // printf("- Node1 valid: %d, has Transform3D: %d\n", ecs_is_valid(world, node1), ecs_has(world, node1, Transform3D));
    
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
    // printf("Node2 entity ID: %llu (%s)\n", (unsigned long long)node2, ecs_get_name(world, node2));
    // printf("- Node2 valid: %d, has Transform3D: %d, parent: %s\n",
    //        ecs_is_valid(world, node2), ecs_has(world, node2, Transform3D),
    //        ecs_get_name(world, ecs_get_parent(world, node2)));
    
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
    // printf("Node3 entity ID: %llu (%s)\n", (unsigned long long)node3, ecs_get_name(world, node3));
    // printf("- Node3 valid: %d, has Transform3D: %d, parent: %s\n",
    //      ecs_is_valid(world, node3), ecs_has(world, node3, Transform3D),
    //      ecs_get_name(world, ecs_get_parent(world, node3)));

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
    // printf("Node4 entity ID: %llu (%s)\n", (unsigned long long)node4, ecs_get_name(world, node4));
    // printf("- Node4 valid: %d, has Transform3D: %d, parent: %s\n",
    //        ecs_is_valid(world, node4), ecs_has(world, node4, Transform3D),
    //        ecs_get_name(world, ecs_get_parent(world, node4)));


    ecs_entity_t gui = ecs_new(world);
    ecs_set_name(world, gui, "transform_gui");  // Optional: Name for debugging
    ecs_set(world, gui, TransformGUI, {
        .id = node1  // Reference the id entity
    });

    // Register GUI system in the 2D rendering phase
    ECS_SYSTEM(world, GUI_Transform3D_System, RLRender2D1Phase, TransformGUI);

    // Register GUI list system in the 2D rendering phase
    ECS_SYSTEM(world, GUI_Transform3D_List_System, RLRender2D1Phase, TransformGUI);

    //Loop Logic and render
    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    UnloadModel(cube);
    ecs_fini(world);
    CloseWindow();
    return 0;
}

// ray cast
void PickingSystem(ecs_iter_t *it){

}

// select list base on transform3d component
void GUI_Transform3D_List_System(ecs_iter_t *it) {
    TransformGUI *guis = ecs_field(it, TransformGUI, 0);

    for (int i = 0; i < it->count; i++) {
        TransformGUI *gui = &guis[i];

        // Create a query for all entities with Transform3D
        ecs_query_t *query = ecs_query(it->world, {
            .terms = {
                { ecs_id(Transform3D) }
            }
        });

        // Count entities to allocate buffer for names
        int entity_count = 0;
        ecs_iter_t transform_it = ecs_query_iter(it->world, query);
        while (ecs_query_next(&transform_it)) {
            entity_count += transform_it.count;
        }

        // Allocate buffers for entity names and IDs
        ecs_entity_t *entity_ids = (ecs_entity_t *)RL_MALLOC(entity_count * sizeof(ecs_entity_t));
        char **entity_names = (char **)RL_MALLOC(entity_count * sizeof(char *));
        int index = 0;

        // Populate entity names and IDs
        transform_it = ecs_query_iter(it->world, query);
        while (ecs_query_next(&transform_it)) {
            for (int j = 0; j < transform_it.count; j++) {
                const char *name = ecs_get_name(it->world, transform_it.entities[j]);
                entity_names[index] = (char *)RL_MALLOC(256 * sizeof(char));
                snprintf(entity_names[index], 256, "%s", name ? name : "(unnamed)");
                entity_ids[index] = transform_it.entities[j];
                // printf("Entity %d: %s (ID: %llu)\n", index, entity_names[index], (unsigned long long)entity_ids[index]);
                index++;
            }
        }

        // Create a single string for GuiListView (concatenate names with semicolons)
        char *name_list = (char *)RL_MALLOC(entity_count * 256 * sizeof(char));
        name_list[0] = '\0';
        for (int j = 0; j < entity_count; j++) {
            if (j > 0) strcat(name_list, ";");
            strcat(name_list, entity_names[j]);
        }
        // printf("Name list: %s\n", name_list);

        // Draw the list view on the left side
        Rectangle list_rect = {560, 10, 240, 580};
        int scroll_index = 0;
        int prev_selected_index = gui->selectedIndex; // Store previous index for comparison

        // Debug: Print current selected index before GuiListView
        // printf("Before GuiListView - Current selectedIndex: %d\n", gui->selectedIndex);

        GuiGroupBox(list_rect, "Entity List");
        GuiListView(list_rect, name_list, &scroll_index, &gui->selectedIndex);

        // Debug: Print gui->selectedIndex after GuiListView
        // printf("After GuiListView - gui->selectedIndex: %d\n", gui->selectedIndex);

        // Update TransformGUI.id if the selection changed
        if (gui->selectedIndex >= 0 && gui->selectedIndex < entity_count && ecs_is_valid(it->world, entity_ids[gui->selectedIndex])) {
            if (gui->id != entity_ids[gui->selectedIndex] || gui->selectedIndex != prev_selected_index) {
                gui->id = entity_ids[gui->selectedIndex];
                // printf("Selected entity: %s (ID: %llu), Index: %d\n",
                //        entity_names[gui->selectedIndex],
                //        (unsigned long long)gui->id,
                //        gui->selectedIndex);
            } else {
                // printf("No change in selection: %s (ID: %llu), Index: %d\n",
                //        entity_names[gui->selectedIndex],
                //        (unsigned long long)gui->id,
                //        gui->selectedIndex);
            }
        } else {
            // printf("Invalid selection: selectedIndex=%d, entity_count=%d, valid=%d\n",
            //        gui->selectedIndex, entity_count,
            //        gui->selectedIndex < entity_count ? ecs_is_valid(it->world, entity_ids[gui->selectedIndex]) : 0);
        }

        // Clean up
        for (int j = 0; j < entity_count; j++) {
            RL_FREE(entity_names[j]);
        }
        RL_FREE(entity_names);
        RL_FREE(entity_ids);
        RL_FREE(name_list);

        // Free the query
        ecs_query_fini(query);
    }
}

// transform 3d position, rotate, scale.
void GUI_Transform3D_System(ecs_iter_t *it) {
    TransformGUI *guis = ecs_field(it, TransformGUI, 0);  // Field index 0

    for (int i = 0; i < it->count; i++) {
        TransformGUI *gui = &guis[i];

        // Check if the referenced entity exists
        if (!ecs_is_valid(it->world, gui->id)) {
            // Disable GUI if entity doesn’t exist
            continue;
        }

        Transform3D *transform = ecs_get_mut(it->world, gui->id, Transform3D);
        if (transform) {
            // Store original values to initialize sliders
            float posX = transform->position.x;
            float posY = transform->position.y;
            float posZ = transform->position.z;
            Vector3 euler = QuaternionToEuler(transform->rotation);
            float rotX = RAD2DEG * euler.x;
            float rotY = RAD2DEG * euler.y;
            float rotZ = RAD2DEG * euler.z;
            float scaleX = transform->scale.x;
            float scaleY = transform->scale.y;
            float scaleZ = transform->scale.z;

            // Store slider input values
            float newPosX = posX;
            float newPosY = posY;
            float newPosZ = posZ;
            float newRotX = rotX;
            float newRotY = rotY;
            float newRotZ = rotZ;
            float newScaleX = scaleX;
            float newScaleY = scaleY;
            float newScaleZ = scaleZ;

            // Draw raygui controls for position, rotation, and scale
            Rectangle panel = {4, 150, 258, 290};  // GUI panel position and size
            GuiGroupBox(panel, "Transform Controls");

            // Position controls
            GuiLabel((Rectangle){20, 170, 100, 20}, "Position");
            GuiSlider((Rectangle){20, 190, 220, 20}, "X", TextFormat("%.2f", posX), &newPosX, -10.0f, 10.0f);
            GuiSlider((Rectangle){20, 210, 220, 20}, "Y", TextFormat("%.2f", posY), &newPosY, -10.0f, 10.0f);
            GuiSlider((Rectangle){20, 230, 220, 20}, "Z", TextFormat("%.2f", posZ), &newPosZ, -10.0f, 10.0f);

            // Rotation controls (display Euler angles, apply incremental quaternions)
            GuiLabel((Rectangle){20, 260, 100, 20}, "Rotation (degrees)");
            GuiSlider((Rectangle){20, 280, 220, 20}, "X", TextFormat("%.2f", rotX), &newRotX, -180.0f, 180.0f);
            GuiSlider((Rectangle){20, 300, 220, 20}, "Y", TextFormat("%.2f", rotY), &newRotY, -180.0f, 180.0f);
            GuiSlider((Rectangle){20, 320, 220, 20}, "Z", TextFormat("%.2f", rotZ), &newRotZ, -180.0f, 180.0f);

            // Scale controls
            GuiLabel((Rectangle){20, 350, 100, 20}, "Scale");
            GuiSlider((Rectangle){20, 370, 220, 20}, "X", TextFormat("%.2f", scaleX), &newScaleX, 0.1f, 5.0f);
            GuiSlider((Rectangle){20, 390, 220, 20}, "Y", TextFormat("%.2f", scaleY), &newScaleY, 0.1f, 5.0f);
            GuiSlider((Rectangle){20, 410, 220, 20}, "Z", TextFormat("%.2f", scaleZ), &newScaleZ, 0.1f, 5.0f);

            // Check if any slider values changed
            bool changed = false;

            // Update position
            if (newPosX != posX || newPosY != posY || newPosZ != posZ) {
                transform->position = (Vector3){newPosX, newPosY, newPosZ};
                changed = true;
            }

            // Update rotations independently for each axis
            bool changed_rot_x = fabs(newRotX - rotX) > 0.001f;
            bool changed_rot_y = fabs(newRotY - rotY) > 0.001f;
            bool changed_rot_z = fabs(newRotZ - rotZ) > 0.001f;

            // Only one rotation axis can update at a time
            if (newRotX != rotX) {
                float deltaRotX = DEG2RAD * (newRotX - rotX);
                Quaternion rotXQuat = QuaternionFromAxisAngle((Vector3){1, 0, 0}, deltaRotX);
                transform->rotation = QuaternionMultiply(transform->rotation, rotXQuat);
                // changed = true;
                transform->isDirty = true;
                printf("X rotation updated: delta=%.2f degrees\n", newRotX - rotX);
            }
            if (newRotY != rotY) {
                float deltaRotY = DEG2RAD * (newRotY - rotY);
                // float deltaRotY = DEG2RAD * newRotY;
                Quaternion rotYQuat = QuaternionFromAxisAngle((Vector3){0, 1, 0}, deltaRotY);
                transform->rotation = QuaternionMultiply(transform->rotation, rotYQuat);
                // changed = true;
                transform->isDirty = true;
                printf("Y rotation updated: delta=%.2f degrees\n", newRotY - rotY);
            }
            if (newRotZ != rotZ) {
                float deltaRotZ = DEG2RAD * (newRotZ - rotZ);
                Quaternion rotZQuat = QuaternionFromAxisAngle((Vector3){0, 0, 1}, deltaRotZ);
                transform->rotation = QuaternionMultiply(transform->rotation, rotZQuat);
                // changed = true;
                transform->isDirty = true;
                printf("Z rotation updated: delta=%.2f degrees\n", newRotZ - rotZ);
            }

            // Update scale
            if (newScaleX != scaleX || newScaleY != scaleY || newScaleZ != scaleZ) {
                transform->scale = (Vector3){newScaleX, newScaleY, newScaleZ};
                changed = true;
            }

            // Apply changes if any slider was modified
            if (changed) {
                transform->isDirty = true;
                // ecs_modified_id(it->world, gui->id, ecs_id(Transform3D));

                // Debug output to confirm changes
                printf("GUI updated %s: Pos (%.2f, %.2f, %.2f), Rot (%.2f, %.2f, %.2f), Scale (%.2f, %.2f, %.2f)\n",
                       ecs_get_name(it->world, gui->id) ? ecs_get_name(it->world, gui->id) : "(unnamed)",
                       transform->position.x, transform->position.y, transform->position.z,
                       newRotX, newRotY, newRotZ,
                       transform->scale.x, transform->scale.y, transform->scale.z);
            }
        }
    }
}

