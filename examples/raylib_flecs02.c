#include <stdio.h>
#include <flecs.h>
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Define maximum number of entities to track (adjust as needed)
#define MAX_ENTITIES 100

typedef struct {
    float x, y;
} Position;

typedef struct {
    float x, y;
} Velocity;

void Move(ecs_iter_t *it) {
    // printf("move... \n");
    Position *p = ecs_field(it, Position, 0);
    Velocity *v = ecs_field(it, Velocity, 1);
    printf("entity count: %d \n", it->count);
    for (int i = 0; i < it->count; i++) {
        p[i].x += v[i].x * it->delta_time;
        p[i].y += v[i].y * it->delta_time;
    }
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();
    
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ECS_SYSTEM(world, Move, EcsOnUpdate, Position, Velocity);

    // ecs_entity_t Bob = ecs_entity(world, { .name = "Bob" });
    // ecs_set(world, Bob, Position, {0, 0});
    // ecs_set(world, Bob, Velocity, {1, 2});

    // Array to store dynamically created entities
    ecs_entity_t entities[MAX_ENTITIES] = {0};
    int entity_count = 0;

    // Main game loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

        // Add button
        if (GuiButton((Rectangle){ 24, 24, 120, 30 }, "#191#Add")) {
            if (entity_count < MAX_ENTITIES) {
                ecs_entity_t _entity = ecs_new(world);
                ecs_set(world, _entity, Position, {0, 0});
                ecs_set(world, _entity, Velocity, {1, 2});
                entities[entity_count++] = _entity;
                printf("Added entity %u, total entities: %d\n", _entity, entity_count);
            }
        }

        // Remove button
        if (GuiButton((Rectangle){ 150, 24, 120, 30 }, "#191#Remove")) {
            if (entity_count > 0) {
                // Remove the most recently added entity
                ecs_entity_t entity_to_remove = entities[--entity_count];
                ecs_delete(world, entity_to_remove);
                entities[entity_count] = 0; // Clear the slot
                printf("Removed entity %u, total entities: %d\n", entity_to_remove, entity_count);
            }
        }

        

        EndDrawing();

        ecs_progress(world, 0);

        // const Position *p = ecs_get(world, Bob, Position);
        // if (p) {
        //     printf("Bob's position is {%f, %f}\n", p->x, p->y);
        // }
    }

    ecs_fini(world);
    CloseWindow();
    printf("end clean up...\n");

    return 0;
}