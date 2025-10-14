// https://www.flecs.dev/flecs/md_docs_2Queries.html
// 
#include <stdio.h>
#include <flecs.h>
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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

    ecs_entity_t Bob = ecs_entity(world, { .name = "Bob" });
    ecs_set(world, Bob, Position, {0, 0});
    ecs_set(world, Bob, Velocity, {1, 2});

    // Array to store dynamically created entities and their names
    ecs_entity_t entities[MAX_ENTITIES] = {0};
    char entity_names[MAX_ENTITIES][32] = {0};
    int entity_count = 0;

    // Main game loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

        // Add button
        if (GuiButton((Rectangle){ 24, 24, 120, 30 }, "#191#Add")) {
            if (entity_count < MAX_ENTITIES) {
                // Create unique name for new entity
                char name[32];
                snprintf(name, sizeof(name), "Entity_%d", entity_count + 1);
                ecs_entity_t _entity = ecs_entity(world, { .name = name });
                ecs_set(world, _entity, Position, {0, 0});
                ecs_set(world, _entity, Velocity, {1, 2});
                entities[entity_count] = _entity;
                strncpy(entity_names[entity_count], name, sizeof(entity_names[entity_count]));
                entity_count++;
                printf("Added entity %s (%u), total entities: %d\n", name, _entity, entity_count);
            }
        }

        // Display entities and remove buttons
        // int y_offset = 60;
        // for (int i = 0; i < entity_count; i++) {
        //     if (entities[i] != 0) {
        //         char button_label[64];
        //         snprintf(button_label, sizeof(button_label), "Remove %s", entity_names[i]);
        //         if (GuiButton((Rectangle){ 24, y_offset, 150, 30 }, button_label)) {
        //             ecs_delete(world, entities[i]);
        //             printf("Removed entity %s (%u), total entities: %d\n", entity_names[i], entities[i], entity_count - 1);
        //             // Shift array to remove the entity
        //             for (int j = i; j < entity_count - 1; j++) {
        //                 entities[j] = entities[j + 1];
        //                 strncpy(entity_names[j], entity_names[j + 1], sizeof(entity_names[j]));
        //             }
        //             entities[entity_count - 1] = 0;
        //             entity_names[entity_count - 1][0] = '\0';
        //             entity_count--;
        //         }
        //         y_offset += 40;
        //     }
        // }

        // Add this before the main loop
        ecs_query_t *q = ecs_query(world, {
            .terms = {
                { .id = ecs_id(Position) },
                { .id = ecs_id(Velocity) }
            }
        });

        // Replace the entity display loop in the main loop with:
        int y_offset = 60;
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            for (int i = 0; i < it.count; i++) {
                const char *name = ecs_get_name(world, it.entities[i]);
                if (!name) name = "Unnamed";
                char button_label[64];
                snprintf(button_label, sizeof(button_label), "Remove %s", name);
                if (GuiButton((Rectangle){ 24, y_offset, 150, 30 }, button_label)) {
                    ecs_delete(world, it.entities[i]);
                    printf("Removed entity %s (%u)\n", name, it.entities[i]);
                }
                y_offset += 40;
            }
        }
        // ecs_iter_fini(&it);

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