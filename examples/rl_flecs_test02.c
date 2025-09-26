// raylib
// https://github.com/SanderMertens/flecs/blob/master/examples/c/systems/custom_pipeline/src/main.c

#include <stdio.h>
#include <flecs.h>
#include "raylib.h"
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


// Forward declare the system callback
// void RenderText(ecs_iter_t *it);

// System callback: Iterate entities with Text and draw
void RenderText(ecs_iter_t *it) {
    Text *texts = ecs_field(it, Text, 0);  // Field index 0 (after implicit 'this' entity)

    for (int i = 0; i < it->count; i++) {
        Text *t = &texts[i];
        DrawText(t->text, t->x, t->y, t->fontSize, t->color);
    }
}

void RL_Begin_Draw_System(ecs_iter_t *it){
    printf("draw\n");
    BeginDrawing();
}

void RL_Render_System(ecs_iter_t *it){
    ClearBackground(RAYWHITE);
    // DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
}

void RL_Render2_System(ecs_iter_t *it){
    DrawText("Congrats! You created your first window!", 190, 220, 20, RED);
}

void RL_End_Draw_System(ecs_iter_t *it){
    EndDrawing();
}

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    // ecs world
    ecs_world_t *world = ecs_init();

    // Define the Text component (Flecs 4.x: uses ecs_struct_init under the hood)
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Text);

    // Phases must have the EcsPhase tag
    ecs_entity_t RLPHASE_BEGINDRAW = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t RLPHASE_RENDER = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t RLPHASE_ENDRAW = ecs_new_w_id(world, EcsPhase);

    // Phases can (but don't have to) depend on other phases which forces ordering
    ecs_add_pair(world, RLPHASE_BEGINDRAW, EcsDependsOn, EcsOnUpdate);
    ecs_add_pair(world, RLPHASE_RENDER, EcsDependsOn, RLPHASE_BEGINDRAW);
    ecs_add_pair(world, RLPHASE_ENDRAW, EcsDependsOn, RLPHASE_RENDER);

    // Create custom pipeline
    // ecs_entity_t pipeline = ecs_pipeline_init(world, &(ecs_pipeline_desc_t){
    //     .query.terms = {
    //         { .id = EcsSystem }, // mandatory
    //         { .id = RLBEGINDRAW }
    //     }
    // });

    // Configure the world to use the custom pipeline
    // ecs_set_pipeline(world, pipeline);

    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLBeginDrawingSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_BEGINDRAW) )
        }),
        .callback = RL_Begin_Draw_System,
    });
    //test render
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLRenderSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_RENDER) )
        }),
        .callback = RL_Render_System,
    });
    //test render
    // ecs_system(world, {
    //     .entity = ecs_entity(world, {
    //         .name = "RLRenderSystem2",
    //         .add = ecs_ids( ecs_dependson(RLPHASE_RENDER) )
    //     }),
    //     .callback = RL_Render2_System,
    // });

    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLBeginDrawingSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_ENDRAW) )
        }),
        .callback = RL_End_Draw_System,
    });

    // Setup RenderText system: Runs every frame, queries Text entities
    // Flecs 4.x: Builder pattern for query (in: read-only for optimization)
    // ECS_SYSTEM(world, RenderText, RLPHASE_RENDER, [in] Text(ui_text));  // Scoped to "ui_text" entity for specificity
    ECS_SYSTEM(world, RenderText, RLPHASE_RENDER, Text);


    // Create entity and add component with params from DrawText
    // (x=190, y=200, fontSize=20; text and color as literals)
    ecs_entity_t uiText = ecs_new(world);
    ecs_set_name(world, uiText, "ui_text");  // Optional: Name for debugging
    ecs_set(world, uiText, Text, {
        .text = "Congrats! You created your first window!",
        .x = 190,
        .y = 200,
        .fontSize = 20,
        // .color = LIGHTGRAY  // Raylib predefined color
        .color = RED  // Raylib predefined color
    });

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

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