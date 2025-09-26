// https://github.com/SanderMertens/flecs/blob/master/examples/c/hello_world/src/main.c

#include <flecs.h>
#include <stdio.h>

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

    for (int i = 0; i < it->count; i++) {
        p[i].x += v[i].x * it->delta_time;
        p[i].y += v[i].y * it->delta_time;
        printf("pos x: %f y:%f \n", p[i].x, p[i].y);
    }
    
}


// int main(int argc, char *argv[]) {
int main() {

    printf("init world \n");

    ecs_world_t *world = ecs_init();
    
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    // ECS_SYSTEM(world, Move, EcsOnUpdate, Position, [in] Velocity);

    // ecs_entity_t ecs_id(Move) = ecs_system(world, {
    //     .entity = ecs_entity(world, { /* ecs_entity_desc_t */
    //         .name = "Move",
    //         .add = ecs_ids( ecs_dependson(EcsOnUpdate) )
    //     }),
    //     .query.terms = { /* ecs_query_desc_t::terms */
    //         { ecs_id(Position) },
    //         { ecs_id(Velocity), .inout = EcsIn }
    //     }
    //     .callback = Move
    // })

    // Create a system that runs the Move function on entities with Position and Velocity
    // ECS_SYSTEM(world, Move, EcsOnUpdate, Position, Velocity);


    // int count = 0;

    // while (ecs_progress(world, 0)) { // Loop indefinitely, or until a condition is met
    //     // You can add your game logic or rendering here
    //     count++;
    //     if (count > 1000){
    //         break;
    //     }
    // }

    // ecs_progress(world, 0);
    // ecs_progress(world, 0);
    // ecs_progress(world, 0);

    // ecs_run(world, ecs_id(Move), 0.0 /* delta_time */, NULL /* param */);
    // ecs_run(world, ecs_id(Move), 0.0 /* delta_time */, NULL /* param */);
    // ecs_run(world, ecs_id(Move), 0.0 /* delta_time */, NULL /* param */);
    // ecs_run(world, ecs_id(Move), 0.0 /* delta_time */, NULL /* param */);

    /* Register system */
    ECS_SYSTEM(world, Move, EcsOnUpdate, Position, Velocity);

    /* Create an entity with name Bob, add Position and food preference */
    ecs_entity_t Bob = ecs_entity(world, { .name = "Bob" });
    ecs_set(world, Bob, Position, {0, 0});
    ecs_set(world, Bob, Velocity, {1, 2});



    /* Run systems twice. Usually this function is called once per frame */
    ecs_progress(world, 0);
    ecs_progress(world, 0);

    /* See if Bob has moved (he has) */
    const Position *p = ecs_get(world, Bob, Position);
    printf("Bob's position is {%f, %f}\n", p->x, p->y);


    ecs_fini(world); // Clean up the world when done
    printf("end world \n");

    return 0;
}