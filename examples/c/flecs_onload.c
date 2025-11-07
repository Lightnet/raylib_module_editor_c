// loop on load and post load
// read the flecs phase doc
#include <flecs.h>
#include <stdio.h>

// Define a component (optional, but common for systems)
typedef struct {
    int value;
} MyComponent;
ECS_COMPONENT_DECLARE(MyComponent);

// Define the system function
void MyLoadSystem(ecs_iter_t *it) {
    // Access components if needed (e.g., MyComponent *c = ecs_field(it, MyComponent, 0);)
    
    // Perform loading or initialization logic here
    printf("MyLoadSystem executed during Ecs.OnLoad!\n");
    
    // Example: Create an entity with a component
    ecs_entity_t entity = ecs_new(it->world);
    ecs_set(it->world, entity, MyComponent, { .value = 42 });
}

void PostLoadSystem(ecs_iter_t *it) {
    printf("PostLoadSystem executed during EcsPostLoad!\n");
}

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init_w_args(argc, argv);

    // Register the component
    ECS_COMPONENT_DEFINE(world, MyComponent); // ecs_set to components

    // Create the system and assign it to the Ecs.OnLoad phase
    ECS_SYSTEM(world, MyLoadSystem, EcsOnLoad, 0); // The '0' can be a query string if needed
    ECS_SYSTEM(world, PostLoadSystem, EcsPostLoad, 0);

    // Run the application
    ecs_progress(world, 0); // run systems in default pipeline
    ecs_progress(world, 0);
    ecs_progress(world, 0);
    ecs_progress(world, 0);

    // Cleanup
    return ecs_fini(world);
}