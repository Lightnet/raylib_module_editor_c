// raylib 5.5
// flecs v4.1.1
// strange rotation for gui
// y -85 to 85 is cap else it will have strange rotate.
// note that child does not update the changes. which need parent to update.
#include <stdio.h>
#include "ecs_components.h"
#include "module_dev.h"
#include "raygui.h"

// player_input_transform_3d_t component
typedef struct {
    bool isMovementMode;
    bool tabPressed;
    bool moveForward;
    bool moveBackward;
    bool moveLeft;
    bool moveRight;
} player_input_transform_3d_t;
ECS_COMPONENT_DECLARE(player_input_transform_3d_t);

// TransformGUI component
typedef struct {
    ecs_entity_t id;          // Entity to edit
    int selectedIndex;        // Index of the selected entity in the list
} transform_3d_gui_t;
ECS_COMPONENT_DECLARE(transform_3d_gui_t);

// select transform id to move cube
void user_input_system(ecs_iter_t *it) {
    // player_input_transform_3d_t *pi_ctx = ecs_singleton_ensure(it->world, player_input_transform_3d_t);
    // if (!pi_ctx) return;

    player_input_transform_3d_t *pi_ctx = ecs_field(it, player_input_transform_3d_t, 0);
    transform_3d_gui_t *transform_3d_gui = ecs_field(it, transform_3d_gui_t, 1);

    if(!transform_3d_gui){
        return;
    }

    Transform3D *t = ecs_field(it, Transform3D, 2);
    float dt = GetFrameTime();
    //test user input
    for (int i = 0; i < it->count; i++) {
      const char *name = ecs_get_name(it->world, it->entities[i]);
      if (name) {
        bool isFound = false;
        // if (strcmp(name, "NodeParent") == 0) {
        if( it->entities[i] == transform_3d_gui->id){

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

// display info
void render2d_hud_system(ecs_iter_t *it){
  player_input_transform_3d_t *pi_ctx = ecs_singleton_ensure(it->world, player_input_transform_3d_t);
  if (!pi_ctx) return;
  Transform3D *t = ecs_field(it, Transform3D, 0);
  float dt = GetFrameTime();

  // for (int i = 0; i < it->count; i++) {
  //   DrawText(TextFormat("Entity %s Pos: %.2f, %.2f, %.2f", 
  //     ecs_get_name(it->world, it->entities[i]) ? ecs_get_name(it->world, it->entities[i]) : "unnamed", 
  //     t[i].position.x, t[i].position.y, t[i].position.z), 10, 130 + i * 20, 20, DARKGRAY);
  // }
  DrawText(TextFormat("Entity Count: %d", it->count), 10, 10, 20, DARKGRAY);
  DrawText(pi_ctx->isMovementMode ? "Mode: Movement (WASD)" : "Mode: Rotation (QWE/ASD)", 10, 30, 20, DARKGRAY);
  DrawText("Tab: Toggle Mode | R: Reset", 10, 50, 20, DARKGRAY);
  DrawFPS(10, 70);
}

// display transform 3d list
void transform_3D_gui_list_system(ecs_iter_t *it) {
    // transform_3d_gui_t *guis = ecs_field(it, transform_3d_gui_t, 0);
    transform_3d_gui_t *gui = ecs_field(it, transform_3d_gui_t, 0);
    // printf("gui\n");

    // for (int i = 0; i < it->count; i++) {
        // transform_3d_gui_t *gui = &guis[i];

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
        Rectangle list_rect = {560, 20, 240, 200};
        int scroll_index = 0;
        int prev_selected_index = gui->selectedIndex; // Store previous index for comparison

        // Debug: Print current selected index before GuiListView
        // printf("Before GuiListView - Current selectedIndex: %d\n", gui->selectedIndex);

        
        GuiListView(list_rect, name_list, &scroll_index, &gui->selectedIndex);

        // Debug: Print gui->selectedIndex after GuiListView
        // printf("After GuiListView - gui->selectedIndex: %d\n", gui->selectedIndex);

        // Update transform_3d_gui_t.id if the selection changed
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
        GuiGroupBox(list_rect, "Entity List");

        // Clean up
        for (int j = 0; j < entity_count; j++) {
            RL_FREE(entity_names[j]);
        }
        RL_FREE(entity_names);
        RL_FREE(entity_ids);
        RL_FREE(name_list);

        // Free the query
        ecs_query_fini(query);
    // }
}

// transform 3d position, rotate, scale.
void transform_3D_gui_system(ecs_iter_t *it) {
    transform_3d_gui_t *gui = ecs_field(it, transform_3d_gui_t, 0);  // Field index 0

    // Check if the referenced entity exists
    if (!ecs_is_valid(it->world, gui->id)) {
        // Disable GUI if entity doesn’t exist
        return;
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
        Rectangle panel = {4, 150, 264, 290};  // GUI panel position and size
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

// draw raylib grid
void render_3d_grid(ecs_iter_t *it){
    DrawGrid(10, 1.0f);
}

// main
int main(void) {
    InitWindow(800, 600, "Transform Hierarchy with Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    // Initialize components and phases
    module_init_raylib(world);
    module_init_dev(world);

    ECS_COMPONENT_DEFINE(world, player_input_transform_3d_t);
    ECS_COMPONENT_DEFINE(world, transform_3d_gui_t);

    ECS_SYSTEM(world, render_3d_grid, RLRender3DPhase);

    // INPUT Current Entity transform3d
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "user_input_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(player_input_transform_3d_t), .src.id = ecs_id(player_input_transform_3d_t)  }, // Singleton source
            { .id = ecs_id(transform_3d_gui_t), .src.id = ecs_id(transform_3d_gui_t)  }, // Singleton source
            { .id = ecs_id(Transform3D), .src.id = EcsSelf }
        },
        .callback = user_input_system
    });

    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "render2d_hud_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
      .query.terms = {
          { .id = ecs_id(Transform3D), .src.id = EcsSelf },
          { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot }
      },
      .callback = render2d_hud_system
    });

    // transform 3d controls
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "transform_3D_gui_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
      .query.terms = {
        { .id = ecs_id(transform_3d_gui_t), .src.id = ecs_id(transform_3d_gui_t)  } // Singleton source
      },
      .callback = transform_3D_gui_system
    });

    // transform 3d list
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "transform_3D_gui_list_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
      .query.terms = {
          { .id = ecs_id(transform_3d_gui_t), .src.id = ecs_id(transform_3d_gui_t)  } // Singleton source
      },
      .callback = transform_3D_gui_list_system
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

    // setup Input
    ecs_singleton_set(world, player_input_transform_3d_t, {
      .isMovementMode=true,
      .tabPressed=false,
      .moveForward=false,
      .moveBackward=false,
      .moveLeft=false,
      .moveRight=false
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

    // set select entity id
    ecs_singleton_set(world, transform_3d_gui_t, {
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
