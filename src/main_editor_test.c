// raylib 5.5
// flecs v4.1.1
// strange rotation for gui
// y -85 to 85 is cap else it will have strange rotate.
// note that child does not update the changes. which need parent to update.
#include <stdio.h>
#include "ecs_components.h"
#include "module_dev.h"
#include "raygui.h"

int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 600;
#define MOUSE_SENSITIVITY 0.002f
#define MOVE_SPEED 5.0f
#define SPRINT_MULTIPLIER 2.0f
#define CAMERA_FOV 60.0f
#define CAMERA_MIN_DISTANCE 0.1f
#define MAX_PITCH 89.0f * DEG2RAD

//===============================================
// COMPONENTS
//===============================================

// Define the name_t struct
typedef struct {
    float yaw;
    float pitch;
    //bool is_capture;
} camera_controller_t;
ECS_COMPONENT_DECLARE(camera_controller_t);

//handle select 3d transform scene
typedef struct {
    ecs_entity_t id;
} select_transform_3d_t;
ECS_COMPONENT_DECLARE(select_transform_3d_t);

// picking ray cast
typedef struct {
    Ray ray;
    RayCollision collision;
} picking_t;
ECS_COMPONENT_DECLARE(picking_t);

typedef struct {
    Vector3 position;
    float width;
    float height;
    float length;
    Color color;
} cube_wire_t;
ECS_COMPONENT_DECLARE(cube_wire_t);

typedef struct {
    ecs_entity_t id;  // Entity to edit (e.g., cube with CubeWire)
    int selectedIndex; // Index of the selected entity in the list
    // Later: Add fields for other GUI controls
} transform_3d_gui_t;
ECS_COMPONENT_DECLARE(transform_3d_gui_t);

typedef struct {
    Model* model;             // Pointer to Model
} block_t;
ECS_COMPONENT_DECLARE(block_t);

typedef struct {
    Model *grass;
    Model *dirt;
    Model *stone;
    Model *rock;
} blocks_t;
ECS_COMPONENT_DECLARE(blocks_t);

//===============================================
// CUBE HELPER
//===============================================

// Function to create a cube mesh with custom UVs for different textures per face
Mesh GenMeshCubeCustomUV(float width, float height, float length, int tileIndices[6]){
    Mesh mesh = { 0 };
    mesh.vertexCount = 24;  // 4 vertices per face × 6 faces
    mesh.triangleCount = 12;  // 2 triangles per face × 6 faces
    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

    float w = width * 0.5f;
    float h = height * 0.5f;
    float l = length * 0.5f;

    // Atlas is 64x64, tiles are 16x16, so UVs are in [0,1] normalized coords
    float tileSize = 16.0f / 64.0f;  // 16/64 = 0.25 (normalized tile size)

    // Vertex data for each face (x, y, z)
    float vertices[] = {
        // Front (+Z)
        -w, -h, l,  -w, h, l,   w, h, l,    w, -h, l,
        // Back (-Z)
        w, -h, -l,  w, h, -l,   -w, h, -l,  -w, -h, -l,
        // Left (-X)
        -w, -h, -l, -w, h, -l,  -w, h, l,   -w, -h, l,
        // Right (+X)
        w, -h, l,   w, h, l,    w, h, -l,   w, -h, -l,
        // Top (+Y)
        -w, h, l,   -w, h, -l,  w, h, -l,   w, h, l,
        // Bottom (-Y)
        -w, -h, -l, -w, -h, l,  w, -h, l,   w, -h, -l
    };

    // UVs: Map each face to a specific 16x16 tile based on tileIndices
    float texcoords[48];
    for (int face = 0; face < 6; face++) {
        int tileIndex = tileIndices[face];
        if (tileIndex < 0 || tileIndex > 15) tileIndex = 0;  // Fallback to tile 0
        float u0 = (tileIndex % 4) * tileSize;  // Left edge of tile
        float v0 = (tileIndex / 4) * tileSize;  // Top edge of tile
        float u1 = u0 + tileSize;               // Right edge
        float v1 = v0 + tileSize;               // Bottom edge
        int i = face * 8;  // 4 vertices × 2 coords (u,v)
        texcoords[i + 0] = u0; texcoords[i + 1] = v1;  // Bottom-left
        texcoords[i + 2] = u0; texcoords[i + 3] = v0;  // Top-left
        texcoords[i + 4] = u1; texcoords[i + 5] = v0;  // Top-right
        texcoords[i + 6] = u1; texcoords[i + 7] = v1;  // Bottom-right
    }

    // Indices: Two triangles per face, reversed for correct winding (clockwise)
    unsigned short indices[] = {
        0, 3, 1,  1, 3, 2,      // Front
        4, 7, 5,  5, 7, 6,      // Back
        8, 11, 9, 9, 11, 10,    // Left
        12, 15, 13, 13, 15, 14, // Right
        16, 19, 17, 17, 19, 18, // Top
        20, 23, 21, 21, 23, 22  // Bottom
    };

    memcpy(mesh.vertices, vertices, mesh.vertexCount * 3 * sizeof(float));
    memcpy(mesh.texcoords, texcoords, mesh.vertexCount * 2 * sizeof(float));
    memcpy(mesh.indices, indices, mesh.triangleCount * 3 * sizeof(unsigned short));

    UploadMesh(&mesh, false);  // Upload to GPU
    return mesh;
}

// Update UVs for an existing mesh without rebuilding
void UpdateMeshUVs(Mesh* mesh, int tileIndices[6]){
    float tileSize = 16.0f / 64.0f;  // 16/64 = 0.25
    float* texcoords = (float*)MemAlloc(mesh->vertexCount * 2 * sizeof(float));
    
    for (int face = 0; face < 6; face++) {
        int tileIndex = tileIndices[face];
        if (tileIndex < 0 || tileIndex > 15) tileIndex = 0;
        float u0 = (tileIndex % 4) * tileSize;
        float v0 = (tileIndex / 4) * tileSize;
        float u1 = u0 + tileSize;
        float v1 = v0 + tileSize;
        int i = face * 8;
        texcoords[i + 0] = u0; texcoords[i + 1] = v1;
        texcoords[i + 2] = u0; texcoords[i + 3] = v0;
        texcoords[i + 4] = u1; texcoords[i + 5] = v0;
        texcoords[i + 6] = u1; texcoords[i + 7] = v1;
    }

    memcpy(mesh->texcoords, texcoords, mesh->vertexCount * 2 * sizeof(float));
    MemFree(texcoords);
    UpdateMeshBuffer(*mesh, 1, mesh->texcoords, mesh->vertexCount * 2 * sizeof(float), 0);  // Update UVs
}

//===============================================
// SYSTEMS
//===============================================

// camera input
void camera_input_system(ecs_iter_t *it){
    main_context_t *main_context = ecs_field(it, main_context_t, 0);
    camera_controller_t *camera_controller = ecs_field(it, camera_controller_t, 1);

    if(IsKeyPressed(KEY_F1)){
        if (IsCursorHidden()) EnableCursor();
        else DisableCursor();
    }

    // if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
    //     if (IsCursorHidden()) EnableCursor();
    //     else DisableCursor();
    // }

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
    picking_t *picking = ecs_field(it, picking_t, 1);
    cube_wire_t *cube_wire = ecs_field(it, cube_wire_t, 2);

    // Ray ray = { 0 };                    // Picking line ray
    // RayCollision collision = { 0 };     // Ray collision hit info

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
        picking->ray = GetScreenToWorldRay(GetMousePosition(), main_context->camera);
        // Iterate matched entities
        for (int i = 0; i < it->count; i++) {
            DrawCubeWires(cube_wire[i].position, cube_wire[i].width,cube_wire[i].height,cube_wire[i].length, cube_wire[i].color);
            picking->collision = GetRayCollisionBox(picking->ray,
                (BoundingBox){(Vector3){ cube_wire[i].position.x - cube_wire[i].width/2, cube_wire[i].position.y - cube_wire[i].height/2, cube_wire[i].position.z - cube_wire[i].length/2 },
                            (Vector3){ cube_wire[i].position.x + cube_wire[i].width/2, cube_wire[i].position.y + cube_wire[i].height/2, cube_wire[i].position.z + cube_wire[i].length/2 }} );
            if (picking->collision.hit){
                cube_wire[i].color = GREEN;
                Vector3 place = Vector3Add(cube_wire[i].position, Vector3Scale(picking->collision.normal, 1.0f));

                ecs_entity_t cube = ecs_new(it->world);
                ecs_set(it->world, cube, cube_wire_t, {
                    .position = place,
                    .width = 1.0f,
                    .height = 1.0f,
                    .length = 1.0f,
                    .color = BLUE
                });

            }else{
                cube_wire[i].color = GRAY;
            }
            // DrawRay(ray, MAROON);
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
        picking->ray = GetScreenToWorldRay(GetMousePosition(), main_context->camera);
        // Iterate matched entities
        for (int i = 0; i < it->count; i++) {
            DrawCubeWires(cube_wire[i].position, cube_wire[i].width,cube_wire[i].height,cube_wire[i].length, cube_wire[i].color);
            picking->collision = GetRayCollisionBox(picking->ray,
                (BoundingBox){(Vector3){ cube_wire[i].position.x - cube_wire[i].width/2, cube_wire[i].position.y - cube_wire[i].height/2, cube_wire[i].position.z - cube_wire[i].length/2 },
                            (Vector3){ cube_wire[i].position.x + cube_wire[i].width/2, cube_wire[i].position.y + cube_wire[i].height/2, cube_wire[i].position.z + cube_wire[i].length/2 }} );
            // if (picking->collision.hit && cube_wire[i].position.x != 0 && cube_wire[i].position.y != 0 && cube_wire[i].position.z != 0){
            if (picking->collision.hit){
                // cube_wire[i].color = YELLOW;
                if (cube_wire[i].position.x == 0.0f && cube_wire[i].position.y == 0.0f && cube_wire[i].position.z == 0.0f){

                }else{
                    ecs_delete(it->world ,it->entities[i]);
                }
            }else{
                // cube_wire[i].color = GRAY;
            }
            // DrawRay(ray, MAROON);
        }
    }
}

//draw crosshair
void render_2d_draw_cross_point(ecs_iter_t *it){
    DrawCircleLines((int)(WINDOW_WIDTH/2), (int)(WINDOW_HEIGHT/2), 8, DARKBLUE);
}

void on_add_entity(ecs_iter_t *it){
    printf("add entity\n");
}

void on_remove_entity(ecs_iter_t *it){
}

// draw raylib grid
void render_3d_grid(ecs_iter_t *it){
    DrawGrid(10, 1.0f);
    const picking_t *picking = ecs_singleton_get(it->world,picking_t);
    if(picking){
        // printf("picking\n");
        DrawRay(picking->ray, MAROON);
        if(picking->collision.hit){
            // Draw a line representing the normal at the hit point
            DrawLine3D(picking->collision.point, Vector3Add(picking->collision.point, Vector3Scale(picking->collision.normal, 1.0f)), RED);
            DrawSphere(picking->collision.point, 0.1f, RED); // Mark the hit point
        }
    }
}

//===============================================
// TRANSFORM 3D
//===============================================

void transform_3D_gui_list_system(ecs_iter_t *it) {
    transform_3d_gui_t *gui = ecs_field(it, transform_3d_gui_t, 0);

    // Create a query for all entities with Transform3D
    ecs_query_t *query = ecs_query(it->world, {
        .terms = {{ ecs_id(Transform3D) }}
    });

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
            index++;
        }
    }

    // Create a single string for GuiListView
    char *name_list = (char *)RL_MALLOC(entity_count * 256 * sizeof(char));
    name_list[0] = '\0';
    for (int j = 0; j < entity_count; j++) {
        if (j > 0) strcat(name_list, ";");
        strcat(name_list, entity_names[j]);
    }

    // Draw the list view on the right side
    Rectangle list_rect = {520, 18, 240, 200}; // Reduced height for more controls
    int scroll_index = 0;
    
    GuiListView(list_rect, name_list, &scroll_index, &gui->selectedIndex);

    // Draw transform controls if an entity is selected
    if (gui->selectedIndex >= 0 && gui->selectedIndex < entity_count && ecs_is_valid(it->world, entity_ids[gui->selectedIndex])) {
        gui->id = entity_ids[gui->selectedIndex];
        Transform3D *transform = ecs_get_mut(it->world, gui->id, Transform3D);
        bool modified = false;

        if (transform) {
            Rectangle control_rect = {520, 228, 240, 250};
            GuiGroupBox(control_rect, "Transform Controls");

            // Position controls
            GuiLabel((Rectangle){530, 230, 100, 20}, "Position");
            float new_pos_x = transform->position.x;
            float new_pos_y = transform->position.y;
            float new_pos_z = transform->position.z;
            GuiSlider((Rectangle){530, 250, 200, 20}, "X", TextFormat("%.2f", new_pos_x), &new_pos_x, -10.0f, 10.0f);
            GuiSlider((Rectangle){530, 270, 200, 20}, "Y", TextFormat("%.2f", new_pos_y), &new_pos_y, -10.0f, 10.0f);
            GuiSlider((Rectangle){530, 290, 200, 20}, "Z", TextFormat("%.2f", new_pos_z), &new_pos_z, -10.0f, 10.0f);
            if (new_pos_x != transform->position.x || new_pos_y != transform->position.y || new_pos_z != transform->position.z) {
                transform->position.x = new_pos_x;
                transform->position.y = new_pos_y;
                transform->position.z = new_pos_z;
                modified = true;
            }

            // Rotation controls (using Euler angles)
            GuiLabel((Rectangle){530, 310, 100, 20}, "Rotation");
            Vector3 euler = QuaternionToEuler(transform->rotation);
            euler.x = RAD2DEG * euler.x;
            euler.y = RAD2DEG * euler.y;
            euler.z = RAD2DEG * euler.z;
            float new_rot_x = euler.x;
            float new_rot_y = euler.y;
            float new_rot_z = euler.z;
            GuiSlider((Rectangle){530, 330, 200, 20}, "X", TextFormat("%.2f", new_rot_x), &new_rot_x, -180.0f, 180.0f);
            GuiSlider((Rectangle){530, 350, 200, 20}, "Y", TextFormat("%.2f", new_rot_y), &new_rot_y, -180.0f, 180.0f);
            GuiSlider((Rectangle){530, 370, 200, 20}, "Z", TextFormat("%.2f", new_rot_z), &new_rot_z, -180.0f, 180.0f);
            if (new_rot_x != euler.x || new_rot_y != euler.y || new_rot_z != euler.z) {
                transform->rotation = QuaternionFromEuler(DEG2RAD * new_rot_x, DEG2RAD * new_rot_y, DEG2RAD * new_rot_z);
                modified = true;
            }

            // Scale controls
            GuiLabel((Rectangle){530, 390, 100, 20}, "Scale");
            float new_scale_x = transform->scale.x;
            float new_scale_y = transform->scale.y;
            float new_scale_z = transform->scale.z;
            GuiSlider((Rectangle){530, 410, 200, 20}, "X", TextFormat("%.2f", new_scale_x), &new_scale_x, 0.1f, 5.0f);
            GuiSlider((Rectangle){530, 430, 200, 20}, "Y", TextFormat("%.2f", new_scale_y), &new_scale_y, 0.1f, 5.0f);
            GuiSlider((Rectangle){530, 450, 200, 20}, "Z", TextFormat("%.2f", new_scale_z), &new_scale_z, 0.1f, 5.0f);
            if (new_scale_x != transform->scale.x || new_scale_y != transform->scale.y || new_scale_z != transform->scale.z) {
                transform->scale.x = new_scale_x;
                transform->scale.y = new_scale_y;
                transform->scale.z = new_scale_z;
                modified = true;
            }

            // Mark transform and descendants as dirty if modified
            if (modified) {
                transform->isDirty = true;
                // UpdateChildTransformOnly(it->world, gui->id); // Update child and descendants immediately
                // ecs_modified(it->world, gui->id, Transform3D);

                // Mark all descendants as dirty
                // ecs_iter_t child_it = ecs_children(it->world, gui->id);
                // while (ecs_children_next(&child_it)) {
                //     for (int j = 0; j < child_it.count; j++) {
                //         if (ecs_has(child_it.world, child_it.entities[j], Transform3D)) {
                //             Transform3D *child_transform = ecs_get_mut(child_it.world, child_it.entities[j], Transform3D);
                //             if (child_transform) {
                //                 child_transform->isDirty = true;
                //                 ecs_modified(child_it.world, child_it.entities[j], Transform3D);
                //             }
                //         }
                //     }
                // }
            }
        }
    }
    list_rect.y -= 8.0f;
    list_rect.height += 8.0f;
    GuiGroupBox(list_rect, "Entity List");

    // Clean up
    for (int j = 0; j < entity_count; j++) {
        RL_FREE(entity_names[j]);
    }
    RL_FREE(entity_names);
    RL_FREE(entity_ids);
    RL_FREE(name_list);
    ecs_query_fini(query);
}


//===============================================
// BLOCKS systems
//===============================================


// Camera3d system for 3d model
void render_3d_blocks_system(ecs_iter_t *it) {
    //filter components from entities
    Transform3D *t = ecs_field(it, Transform3D, 0);
    block_t *block = ecs_field(it, block_t, 1);

    for (int i = 0; i < it->count; i++) {
        ecs_entity_t entity = it->entities[i];
        if (!ecs_is_valid(it->world, entity)) {
            //printf("Skipping invalid entity ID: %llu\n", (unsigned long long)entity);
            continue;
        }
        const char *name = ecs_get_name(it->world, entity) ? ecs_get_name(it->world, entity) : "(unnamed)";
        if (!ecs_has(it->world, entity, Transform3D) || !ecs_has(it->world, entity, block_t)) {
            //printf("Skipping entity %s: missing Transform3D or block_t\n", name);
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
        if (!block[i].model) {
            // printf("Skipping entity %s: null model\n", name);
            continue;
        }
        block[i].model->transform = t[i].worldMatrix;
        bool isChild = ecs_has_pair(it->world, entity, EcsChildOf, EcsWildcard);
        DrawModel(*(block[i].model), (Vector3){0, 0, 0}, 1.0f, WHITE);
        //   DrawModelWires(*(block[i].model), (Vector3){0, 0, 0}, 1.0f, GREEN); 
    }
}

// clean up for blocks
void on_remove_block_system(ecs_iter_t *it){
    // block_t *block = ecs_field(it, block_t, 0);
    for (int i = 0; i < it->count; i ++) {
        ecs_entity_t current_entity = it->entities[i];
        const block_t *block = ecs_get(it->world, current_entity, block_t);
        if(block){
            printf("remove block_t\n");
            // this handle clean up
            // dBodyDestroy(ode_body->id);
            if(block->model){
                UnloadModel(*(block->model));
            }
        }
    }
}

//===============================================
// MAIN
//===============================================
int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Editor Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    // Initialize components and phases
    module_init_raylib(world);
    module_init_dev(world);

    ECS_COMPONENT_DEFINE(world, camera_controller_t);
    ECS_COMPONENT_DEFINE(world, cube_wire_t);
    ECS_COMPONENT_DEFINE(world, picking_t);
    ECS_COMPONENT_DEFINE(world, select_transform_3d_t);
    ECS_COMPONENT_DEFINE(world, transform_3d_gui_t);

    ECS_COMPONENT_DEFINE(world, blocks_t);
    ECS_COMPONENT_DEFINE(world, block_t);

    ECS_SYSTEM(world, render_3d_grid, RLRender3DPhase);

    // camera input
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "camera_input_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
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

    // picking raycast system
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "cube_wires_picking_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) }, // Singleton
            { .id = ecs_id(picking_t), .src.id = ecs_id(picking_t) }, // Singleton
            { .id = ecs_id(cube_wire_t) },   //
        },
        .callback = cube_wires_picking_system
    });

    // draw center crosshair
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_2d_draw_cross_point", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
        .callback = render_2d_draw_cross_point
    });

    // render 3d mesh
    ecs_system(world, {
        .entity = ecs_entity(world, {
          .name = "render_3d_blocks_system", 
          .add = ecs_ids(ecs_dependson(RLRender3DPhase))
        }),
        .query.terms = {
            { .id = ecs_id(Transform3D), .src.id = EcsSelf },
            { .id = ecs_id(block_t), .src.id = EcsSelf }
        },
        .callback = render_3d_blocks_system
    });

    // Register GUI list system in the 2D rendering phase
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "transform_3D_gui_list_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
        .query.terms = {
            { .id = ecs_id(transform_3d_gui_t), .src.id = ecs_id(transform_3d_gui_t) } // Singleton
        },
        .callback = transform_3D_gui_list_system
    });

    // Create observer that is invoked whenever Position is set
    ecs_observer(world, {
        .query.terms = {{ ecs_id(Transform3D) }},
        .events = { EcsOnAdd },
        .callback = on_add_entity
    });

    // Create observer that is invoked whenever block_t model is remove
    ecs_observer(world, {
        .query.terms = {{ ecs_id(block_t) }},
        .events = { EcsOnRemove },
        .callback = on_remove_block_system
    });

    //===========================================
    // CAMERA and others
    //===========================================
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

    ecs_singleton_set(world,picking_t,{
        .ray = {0},
        .collision = {0}
    });

    //===========================================
    // Models
    //===========================================

    // create Model
    Model model_cube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));



    // Load texture atlas (ensure resources/simple_color64x64.png exists)
    Image atlasImage = LoadImage("resources/altas_texture64x64.png");
    if (atlasImage.data == NULL) {
        TraceLog(LOG_ERROR, "Failed to load texture: resources/altas_texture64x64.png");
        CloseWindow();
        return 1;
    }
    ImageFormat(&atlasImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);  // Ensure RGBA
    Texture2D atlasTexture = LoadTextureFromImage(atlasImage);
    UnloadImage(atlasImage);  // Free image

    // Tile indices for each face (front, back, left, right, top, bottom)
    // int tileIndices[6] = { 0, 1, 2, 3, 4, 5 };  // Example: different tiles per face
    int tileIndices[6] = { 1, 1, 1, 1, 0, 2 };  // Example: different tiles per face
    // 1 = side
    // 2 = side
    // 3 = side
    // 4 = side
    // 5 = top
    // 6 = bottom

    // Create cube mesh and model
    Mesh cubeMesh = GenMeshCubeCustomUV(1.0f, 1.0f, 1.0f, tileIndices);
    Model cubeModel = LoadModelFromMesh(cubeMesh);
    SetMaterialTexture(&cubeModel.materials[0], MATERIAL_MAP_DIFFUSE, atlasTexture);



    //===========================================
    // ENTITY
    //===========================================

    ecs_entity_t cube_wired_1 = ecs_entity(world, {
      .name = "CubeWire1"
    });
    ecs_set(world, cube_wired_1, cube_wire_t, {
        .position = (Vector3) {0,2.0f,0},
        .width = 1.0f,
        .height = 1.0f,
        .length = 1.0f,
        .color = BLUE
    });

    ecs_entity_t cube_1 = ecs_entity(world, {
      .name = "cube_1"
    });
    ecs_set(world, cube_1, Transform3D, {
        .position = (Vector3){1.0f, 0.0f, 0.0f},
        .rotation = QuaternionIdentity(),
        .scale = (Vector3){1.0f, 1.0f, 1.0f},
        .localMatrix = MatrixIdentity(),
        .worldMatrix = MatrixIdentity(),
        .isDirty = true
    });
    // ecs_set(world, cube_1, ModelComponent, {&model_cube});
    // ecs_set(world, cube_1, block_t, {&model_cube});
    ecs_set(world, cube_1, block_t, {&cubeModel});


    ecs_singleton_set(world, transform_3d_gui_t, {
        .id = cube_1  // Reference the id entity
    });





    // setup Input
    // ecs_singleton_set(world, PlayerInput_T, {
    //   .isMovementMode=true,
    //   .tabPressed=false
    // });

    // create Model
    // Model cube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));

    // // Create Entity
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
