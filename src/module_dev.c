#include "module_dev.h"
#include "ecs_components.h"
#include <stdio.h>

// #define RAYGUI_IMPLEMENTATION
#include "raygui.h"

void on_add_entity_dev(ecs_iter_t *it){

}

void on_remove_entity_dev(ecs_iter_t *it){

}

void render2d_hud_debug_system(ecs_iter_t *it){

    DrawText(TextFormat("test ..."), 2, 2, 20, DARKGRAY);
}

void setup_systems_dev(ecs_world_t *world){

    // ONLY 2D
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "render2d_hud_debug_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
      //.query.terms = {
        //   { .id = ecs_id(Transform3D), .src.id = EcsSelf },
        //   { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot }
      //},
      .callback = render2d_hud_debug_system
    });

    

}

// void setup_components_dev(ecs_world_t *world){

// }

void module_init_dev(ecs_world_t *world){

    // Define components
    // setup_components_dev(world);
    //systems
    setup_systems_dev(world);

}