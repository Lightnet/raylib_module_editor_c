// module_enet.c
#include <stdio.h>
#include "module_enet.h"
#include "ecs_components.h" // phase
#include "raylib.h"
#include "raygui.h"

// Declare and define component in the source file
ECS_COMPONENT_DECLARE(enet_packet_t);


// Declare event entity (defined in setup_components_enet)
// ecs_entity_t event_receive_packed = 0;
ecs_entity_t event_receive_packed;

/*

- system set up for server or client
- system loop with condtions
- system pass event for flecs
- system filter 

*/

void render2d_hud_enet_system(ecs_iter_t *it){

    DrawText(TextFormat("test ..."), 2, 2, 20, DARKGRAY);
}

void on_receive_packed(ecs_iter_t *it) {
    printf("event on_receive_packed\n");
    enet_packet_t *p = it->param;
    if (p && p->data) {
        char *str = (char*)p->data;
        printf("Received string: %s\n", str);
        free(str); // Free the dynamically allocated string
    } else {
        printf("No valid string data received\n");
    }
}

void test_input_enet_system(ecs_iter_t *it){
    if (IsKeyDown(KEY_R)) {
        char *str = strdup("hello"); // Dynamically allocate string
        enet_packet_t packet = { .data = str };
        ecs_emit(it->world, &(ecs_event_desc_t) {
            .event = ecs_id(enet_packet_t),
            .entity = event_receive_packed,
            .param = &packet
        });
        // Note: str must be freed later, e.g., in the observer or a cleanup system
    }
}


void setup_systems_enet(ecs_world_t *world){

    // Input
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "test_input_enet_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
      //.query.terms = {
        //   { .id = ecs_id(Transform3D), .src.id = EcsSelf },
        //   { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot }
      //},
      .callback = test_input_enet_system
    });

    // ONLY 2D
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "render2d_hud_debug_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
      //.query.terms = {
        //   { .id = ecs_id(Transform3D), .src.id = EcsSelf },
        //   { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot }
      //},
      .callback = render2d_hud_enet_system
    });

    // Create an entity observer
    ecs_observer(world, {
        // Not interested in any specific component
        .query.terms = {{ EcsAny, .src.id = event_receive_packed }},
        .events = { ecs_id(enet_packet_t) },
        .callback = on_receive_packed
    });
}

void setup_components_enet(ecs_world_t *world){
    // Define the enet_packet_t component (called only once)
    ECS_COMPONENT_DEFINE(world, enet_packet_t);

    // Define the event entity
    event_receive_packed = ecs_entity(world, { .name = "receive_packed" });
}


void module_init_enet(ecs_world_t *world){

    // Define components
    setup_components_enet(world);
    //systems
    setup_systems_enet(world);

    
}