#include <flecs.h>
#include <stdio.h>


void Startup(ecs_iter_t *it) {
    printf("%s\n", ecs_get_name(it->world, it->system));
}

void Update(ecs_iter_t *it) {
    printf("%s\n", ecs_get_name(it->world, it->system));
}

int main(int argc, char *argv[]) {
    ecs_world_t *ecs = ecs_init_w_args(argc, argv);

    // Startup system
    ECS_SYSTEM(ecs, Startup, EcsOnStart, 0);

    // Regular system
    ECS_SYSTEM(ecs, Update, EcsOnUpdate, 0);

    // First frame. This runs both the Startup and Update systems
    ecs_progress(ecs, 0.0f);

    // Second frame. This runs only the Update system
    ecs_progress(ecs, 0.0f);

    return ecs_fini(ecs);
}
