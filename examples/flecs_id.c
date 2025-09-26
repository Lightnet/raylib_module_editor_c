#include <flecs.h>
#include <stdio.h>

// component
typedef struct {
    float x, y;
} Position;

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init();

    // Create a new entity
    ecs_entity_t my_entity = ecs_new(world);

    // my_entity now holds the unique ID for this entity.
    // You can use it directly to refer to the entity.

    // Example: add a component to the entity
    ECS_COMPONENT(world, Position); // Define Position component
    ecs_set(world, my_entity, Position, {10, 20});

    // Example: print the entity ID (which is the Flecs ID)
    printf("Entity ID: %llu\n", (unsigned long long)my_entity); //return number

    ecs_fini(world);
    return 0;
}