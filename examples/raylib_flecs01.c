// 

#include <stdio.h>

#include <flecs.h>
#include "raylib.h"

typedef struct {
    float x, y;
} Position;

typedef struct {
    float x, y;
} Velocity;

void Move(ecs_iter_t *it) {
    printf("move... \n");
    Position *p = ecs_field(it, Position, 0);
    Velocity *v = ecs_field(it, Velocity, 1);
    printf("entity count: %d \n", it->count);
    for (int i = 0; i < it->count; i++) {
        p[i].x += v[i].x * it->delta_time;
        p[i].y += v[i].y * it->delta_time;
        // printf("pos x: %f y:%f \n", p[i].x, p[i].y);
    }
    
}


int main() {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    ecs_world_t *world = ecs_init();
    
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    /* Register system */
    ECS_SYSTEM(world, Move, EcsOnUpdate, Position, Velocity);

    /* Create an entity with name Bob, add Position and food preference */
    ecs_entity_t Bob = ecs_entity(world, { .name = "Bob" });
    ecs_set(world, Bob, Position, {0, 0});
    ecs_set(world, Bob, Velocity, {1, 2});

    // ecs_entity_t parent = ecs_entity(world, {
    //     .name = "parent"
    // });

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

            if (GuiButton((Rectangle){ 24, 24, 120, 30 }, "#191#Add")){
                    // ecs_entity_t _entity = ecs_entity(world, { .name = "Bob" });
                    ecs_entity_t _entity = ecs_new(world);
                    ecs_set(world, _entity, Position, {0, 0});
                    ecs_set(world, _entity, Velocity, {1, 2});
            }
            if (GuiButton((Rectangle){ 24, 24, 120, 30 }, "#191#Remove")){

            }

        EndDrawing();

        ecs_progress(world, 0);

        
        const Position *p = ecs_get(world, Bob, Position);
        printf("Bob's position is {%f, %f}\n", p->x, p->y);

    }

    // De-Initialization
    ecs_fini(world); // Clean up the world when done
    CloseWindow();        // Close window and OpenGL context


    printf("end clean up...\n");

    return 0;
}