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

void RL_Begin_Draw_System(ecs_iter_t *it){
    printf("draw\n");
    BeginDrawing();
}

void RL_Render_System(ecs_iter_t *it){
    ClearBackground(RAYWHITE);
    DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
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

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

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
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLRenderSystem2",
            .add = ecs_ids( ecs_dependson(RLPHASE_RENDER) )
        }),
        .callback = RL_Render2_System,
    });

    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "RLBeginDrawingSystem",
            .add = ecs_ids( ecs_dependson(RLPHASE_ENDRAW) )
        }),
        .callback = RL_End_Draw_System,
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