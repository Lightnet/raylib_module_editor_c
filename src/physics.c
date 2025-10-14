// physics.c
#include "physics.h"

void physics_system(ecs_iter_t *it) {
    // Example physics system
    // Access Transform3D components or other data
}

void physics_init(ecs_world_t *world) {
    // Register physics system in LogicUpdatePhase
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "physics_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {{ .id = ecs_id(Transform3D), .src.id = EcsSelf }},
        .callback = physics_system
    });
}