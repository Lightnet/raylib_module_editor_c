// raylib 5.5
// flecs v4.1
// https://github.com/SanderMertens/flecs/blob/master/examples/c/systems/custom_pipeline/src/main.c
// 
#include <stdio.h>
#include <flecs.h>
#include "raylib.h"
#include <math.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// component
typedef struct {
    float x, y;
} Position;

typedef struct {
    float x, y;
} Velocity;

// Component definition: Matches DrawText params
typedef struct {
    const char* text;
    int x;
    int y;
    int fontSize;
    Color color;
} Text;

typedef struct {
    Vector3 position;  // Cube position
    float width;       // Cube width
    float height;      // Cube height
    float length;      // Cube length
    Color color;       // Cube color
} CubeWire;
ECS_COMPONENT_DECLARE(CubeWire);

typedef struct {
    ecs_entity_t id;  // Entity to edit (e.g., cube with CubeWire)
    // Later: Add fields for other GUI controls
} TransformGUI;
ECS_COMPONENT_DECLARE(TransformGUI);

// Define the SceneContext struct
typedef struct {
    Camera3D camera;
    // tmp physics; (assuming this is a placeholder, you can add actual physics data later)
} SceneContext;

// Forward declare the system callback
// raylib
void RenderText(ecs_iter_t *it);
void RenderCubeWire(ecs_iter_t *it);
void UpdateCubeWire(ecs_iter_t *it);
void GUI_Transform3D_System(ecs_iter_t *it);

// start render gl
void RL_BeginDrawing_System(ecs_iter_t *it);

void RL_Render2D0_System(ecs_iter_t *it);// by default render 2d

// begin render 3d mode
void RL_BeginMode3D_System(ecs_iter_t *it);
void RL_Render3D_System(ecs_iter_t *it);
void RL_EndMode3D_System(ecs_iter_t *it);
// exit 3d mode to 2D mode.
void RL_Render2D1_System(ecs_iter_t *it);
// end render gl
void RL_End_Draw_System(ecs_iter_t *it);

Vector3 cubePosition;

// main
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    cubePosition = (Vector3){ 0.0f, 0.0f, 0.0f };

    // ecs world
    ecs_world_t *world = ecs_init();

    // Define the Text component (Flecs 4.x: uses ecs_struct_init under the hood)
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Text);
    // ECS_COMPONENT(world, CubeWire); //error?
    ECS_COMPONENT_DEFINE(world, CubeWire);
    // ECS_COMPONENT(world, TransformGUI); //error?
    ECS_COMPONENT_DEFINE(world, TransformGUI);

    // Phases must have the EcsPhase tag
    ecs_entity_t RLPHASE_BEGINDRAWING = ecs_new_w_id(world, EcsPhase); // RL BeginDrawing();

    ecs_entity_t RLPHASE_RENDER2D0 = ecs_new_w_id(world, EcsPhase); // RL for gl background color

    ecs_entity_t RLPHASE_BEGINMODE3D = ecs_new_w_id(world, EcsPhase); // BeginMode3D(camera);
    ecs_entity_t RLPHASE_RENDER3D = ecs_new_w_id(world, EcsPhase); // Draw 3D ex. DrawGrid(10, 1.0f);
    ecs_entity_t RLPHASE_ENDMODE3D = ecs_new_w_id(world, EcsPhase); // EndMode3D();

    ecs_entity_t RLPHASE_RENDER2D1 = ecs_new_w_id(world, EcsPhase); // Draw 2D
    ecs_entity_t RLPHASE_ENDRAWING = ecs_new_w_id(world, EcsPhase); // EndDrawing();

    // Phases can (but don't have to) depend on other phases which forces ordering
    ecs_add_pair(world, RLPHASE_BEGINDRAWING, EcsDependsOn, EcsOnUpdate);
    ecs_add_pair(world, RLPHASE_RENDER2D0, EcsDependsOn, RLPHASE_BEGINDRAWING);
    ecs_add_pair(world, RLPHASE_BEGINMODE3D, EcsDependsOn, RLPHASE_RENDER2D0);
    ecs_add_pair(world, RLPHASE_RENDER3D, EcsDependsOn, RLPHASE_BEGINMODE3D);
    ecs_add_pair(world, RLPHASE_ENDMODE3D, EcsDependsOn, RLPHASE_RENDER3D);
    ecs_add_pair(world, RLPHASE_RENDER2D1, EcsDependsOn, RLPHASE_ENDMODE3D);
    ecs_add_pair(world, RLPHASE_ENDRAWING, EcsDependsOn, RLPHASE_RENDER2D1);

    // start gl
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLBeginDrawingSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_BEGINDRAWING) )
        }),
        .callback = RL_BeginDrawing_System,
    });

    // gl 2d for background color
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLRender2D0System",
            .add = ecs_ids( ecs_dependson(RLPHASE_RENDER2D0) )
        }),
        .callback = RL_Render2D0_System,
    });

    // start mode 3d
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLBeginMode3DSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_BEGINMODE3D) )
        }),
        .callback = RL_BeginMode3D_System,
    });

    // render 3d
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLRender3DSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_RENDER3D) )
        }),
        .callback = RL_Render3D_System,
    });
    // end mode 3d
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLEndMode3DSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_ENDMODE3D) )
        }),
        .callback = RL_EndMode3D_System,
    });

    // render 2D
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLRender2DSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_RENDER2D1) )
        }),
        .callback = RL_Render2D1_System,
    });

    // end gl
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLEndDrawingSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_ENDRAWING) )
        }),
        .callback = RL_End_Draw_System,
    });

    // Setup RenderText system: Runs every frame, queries Text entities
    // Flecs 4.x: Builder pattern for query
    ECS_SYSTEM(world, RenderText, RLPHASE_RENDER2D1, Text);
    // Define the RenderCubeWire system to run in the 3D rendering phase
    ECS_SYSTEM(world, RenderCubeWire, RLPHASE_RENDER3D, CubeWire);
    //ECS_SYSTEM(world, UpdateCubeWire, EcsOnUpdate, CubeWire);// test

    // Register GUI system in the 2D rendering phase
    ECS_SYSTEM(world, GUI_Transform3D_System, RLPHASE_RENDER2D1, TransformGUI);

    // Create entity and add component with params from DrawText
    // (x=190, y=200, fontSize=20; text and color as literals)
    ecs_entity_t uiText = ecs_new(world);
    ecs_set_name(world, uiText, "ui_text");  // Optional: Name for debugging
    ecs_set(world, uiText, Text, {
        .text = "Congrats! You created your first window!",
        .x = 10,
        .y = 10,
        .fontSize = 20,
        // .color = LIGHTGRAY  // Raylib predefined color
        .color = RED  // Raylib predefined color
    });

    // entity wire cube
    ecs_entity_t cube = ecs_new(world);
    ecs_set_name(world, cube, "cube");  // Optional: Name for debugging
    ecs_set(world, cube, CubeWire, {
        .position = (Vector3){0.0f, 0.0f, 0.0f},
        .width = 2.0f,
        .height = 2.0f,
        .length = 2.0f,
        .color = MAROON
    });

    ecs_entity_t gui = ecs_new(world);
    ecs_set_name(world, gui, "transform_gui");  // Optional: Name for debugging
    ecs_set(world, gui, TransformGUI, {
        .id = cube  // Reference the cube entity
    });

    printf("Cube ID %llu: No CubeWire component found\n", (unsigned long long)cube);


    // Initialize SceneContext and set it as the ECS context
    SceneContext sceneContext = {
        .camera = {
            .position = (Vector3){10.0f, 10.0f, 10.0f},
            .target = (Vector3){0.0f, 0.0f, 0.0f},
            .up = (Vector3){0.0f, 1.0f, 0.0f},
            .fovy = 45.0f,
            .projection = CAMERA_PERSPECTIVE
        }
        // Initialize physics or other fields here if needed
    };

    // Set SceneContext as the ECS world context
    ecs_set_ctx(world, &sceneContext, NULL);

    // raylib set up
    InitWindow(screenWidth, screenHeight, "raylib flecs v4.1 camera cube");

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        
        // BeginDrawing();
        //     ClearBackground(RAYWHITE);
        //     DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
        //     ecs_progress(world, 0);
        // EndDrawing();
        ecs_progress(world, 0);

    }

    // De-Initialization
    ecs_fini(world);

    CloseWindow();        // Close window and OpenGL context

    return 0;
}
// render gl
void RL_BeginDrawing_System(ecs_iter_t *it){
    // printf("draw\n");
    BeginDrawing();
}
// render 2d
void RL_Render2D0_System(ecs_iter_t *it){
    // background color
    ClearBackground(RAYWHITE);
}
// start mode 3d
void RL_BeginMode3D_System(ecs_iter_t *it){
    // printf("RL_BeginMode3D_System\n");
    // Retrieve the SceneContext from the ECS world
    SceneContext *sceneContext = (SceneContext *)ecs_get_ctx(it->world);
    if (!sceneContext) return;
    // printf("RL_BeginMode3D_System\n");
    // Access the Camera3D from SceneContext
    Camera3D *camera = &sceneContext->camera;
    BeginMode3D(*camera);
}
// render 3d
void RL_Render3D_System(ecs_iter_t *it){
    // printf("RL_Render3D_System\n");
    // DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);
}
// end mode 3D
void RL_EndMode3D_System(ecs_iter_t *it){
    // printf("RL_EndMode3D_System\n");
    // Retrieve the SceneContext from the ECS world
    SceneContext *sceneContext = (SceneContext *)ecs_get_ctx(it->world);
    if (!sceneContext) return;
    // printf("RL_EndMode3D_System\n");
    // End the 3D rendering mode
    EndMode3D();
}
// render 2d
void RL_Render2D1_System(ecs_iter_t *it){
    // ClearBackground(RAYWHITE);
    // DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
    // DrawText("Congrats! You created your first window!", 10, 30, 20, LIGHTGRAY);
}
// end gl
void RL_End_Draw_System(ecs_iter_t *it){
    EndDrawing();
}

// System callback: Iterate entities with Text and draw
void RenderText(ecs_iter_t *it) {
    Text *texts = ecs_field(it, Text, 0);  // Field index 0 (after implicit 'this' entity)

    for (int i = 0; i < it->count; i++) {
        Text *t = &texts[i];
        DrawText(t->text, t->x, t->y, t->fontSize, t->color);
    }
}

// wireframe cube
void RenderCubeWire(ecs_iter_t *it) {
    CubeWire *cubes = ecs_field(it, CubeWire, 0);  // Field index 0

    for (int i = 0; i < it->count; i++) {
        CubeWire *c = &cubes[i];
        DrawCubeWires(c->position, c->width, c->height, c->length, c->color);
    }
}
// test update postion and color
void UpdateCubeWire(ecs_iter_t *it) {
    CubeWire *cubes = ecs_field(it, CubeWire, 0);
    float delta = GetFrameTime();  // raylib function for frame time

    for (int i = 0; i < it->count; i++) {
        CubeWire *c = &cubes[i];
        // Example: Move cube along x-axis
        c->position.x += 1.0f * delta;  // Move 1 unit per second
        if (c->position.x > 5.0f) c->position.x = -5.0f;  // Loop back
        // Example: Change color over time
        c->color.r = (unsigned char)(sinf(GetTime()) * 127.0f + 127.0f);
    }
}


void GUI_Transform3D_System(ecs_iter_t *it) {
    TransformGUI *guis = ecs_field(it, TransformGUI, 0);  // Field index 0

    for (int i = 0; i < it->count; i++) {
        TransformGUI *gui = &guis[i];
        
        // Check if the referenced entity exists
        if (!ecs_is_valid(it->world, gui->id)) {
            // Disable GUI if entity doesn’t exist
            continue;
        }

        // Get the CubeWire component of the referenced entity
        CubeWire *cube = ecs_get_mut(it->world, gui->id, CubeWire);
        if (cube) {
            printf("cube\n");

            // Draw raygui controls for x, y, z
            Rectangle panel = {10, 50, 200, 120};  // GUI panel position and size
            GuiGroupBox(panel, "Transform Controls");

            // Temporary variables for sliders
            float x = cube->position.x;
            float y = cube->position.y;
            float z = cube->position.z;

            // Sliders for x, y, z (range: -10 to 10)
            GuiLabel((Rectangle){20, 70, 50, 20}, "X:");
            GuiSlider((Rectangle){60, 70, 130, 20}, NULL, NULL, &x, -10.0f, 10.0f);
            GuiLabel((Rectangle){20, 100, 50, 20}, "Y:");
            GuiSlider((Rectangle){60, 100, 130, 20}, NULL, NULL, &y, -10.0f, 10.0f);
            GuiLabel((Rectangle){20, 130, 50, 20}, "Z:");
            GuiSlider((Rectangle){60, 130, 130, 20}, NULL, NULL, &z, -10.0f, 10.0f);

            // Update the CubeWire position if changed
            if (x != cube->position.x || y != cube->position.y || z != cube->position.z) {
                cube->position = (Vector3){x, y, z};
                ecs_modified_id(it->world, gui->id, ecs_id(CubeWire));
            }
        }
        
    }
}


