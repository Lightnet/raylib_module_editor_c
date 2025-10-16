# Singleton

Singletons are a special case of fixed source term, where the component is matched on itself. An example:

c
```c
ecs_singleton_set(world, TimeOfDay, {0});
 
// Observer with singleton source
ecs_observer(world, {
    .query.terms = {
        { ecs_id(TimeOfDay), .src.id = ecs_id(TimeOfDay) }
    },
    .events = { EcsOnSet },
    .callback = MyObserver
});
 
// Triggers observer
ecs_singleton_set(world, TimeOfDay, {1});
 
// Does not trigger observer
ecs_entity_t e = ecs_insert(world, ecs_value(TimeOfDay, {0}));
```

Tasks may query for components from a fixed source or singleton:

c
```c
// System function
void PrintTime(ecs_iter_t *it) {
    // Get singleton component
    Game *g = ecs_field(it, Game, 0);
 
    printf("Time: %f\n", g->time);
}
 
// System declaration using the ECS_SYSTEM macro
ECS_SYSTEM(world, PrintTime, EcsOnUpdate, Game($));
 
// System declaration using the descriptor API
ecs_system(world, {
    .entity = ecs_entity(world, {
        .name = "PrintTime",
        .add = ecs_ids( ecs_dependson(EcsOnUpdate) )
    }),
    .query.terms = {
        { .id = ecs_id(Game), .src.id = ecs_id(Game) } // Singleton source
    },
    .callback = PrintTime
});
```

c
```c
#include <flecs.h>

// Define your singleton component
typedef struct GameSettings {
    float master_volume;
    bool fullscreen_enabled;
} GameSettings;

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init();

    // Register the component
    ECS_COMPONENT(world, GameSettings);

    // Set the singleton component's value
    ecs_set_singleton(world, GameSettings, { .master_volume = 0.75f, .fullscreen_enabled = true });

    // ... rest of your application ...

    ecs_fini(world);
    return 0;
}
```


```c
#include <flecs.h>

// Define your singleton component
typedef struct GameSettings {
    float master_volume;
    bool fullscreen_enabled;
} GameSettings;

// Define a regular component for entities
typedef struct Player {
    // ... player data ...
} Player;

// System that uses the singleton GameSettings
void PrintSettings(ecs_iter_t *it) {
    // Access the singleton GameSettings directly
    const GameSettings *settings = ecs_singleton_get(it->world, GameSettings); // read mode

    printf("Master Volume: %.2f, Fullscreen Enabled: %s\n", 
           settings->master_volume, settings->fullscreen_enabled ? "true" : "false");
}

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, GameSettings);
    ECS_COMPONENT(world, Player);

    // Set the singleton component
    ecs_set_singleton(world, GameSettings, { .master_volume = 0.75f, .fullscreen_enabled = true });

    // Create a system that accesses the singleton
    ECS_SYSTEM(world, PrintSettings, EcsOnUpdate, 0); // 0 indicates no specific query for entities

    // Run the world (this will execute the PrintSettings system)
    ecs_progress(world, 0); 

    ecs_fini(world);
    return 0;
}
```
    Read mode only on ecs_singleton_get.

```c
// System that uses the singleton GameSettings
void PrintSettings(ecs_iter_t *it) {
    // Access the singleton GameSettings directly
    GameSettings *settings = ecs_singleton_get_mut(it->world, GameSettings); // write mode

    printf("Master Volume: %.2f, Fullscreen Enabled: %s\n", 
           settings->master_volume, settings->fullscreen_enabled ? "true" : "false");
}
```
    Write mode and read.