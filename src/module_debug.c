// module_debug.c
// for testing raygui 
#include "ecs_components.h"
#include "module_debug.h"
#include "raygui.h"

void render_2d_debug_system(ecs_iter_t *it){
    DrawText(TextFormat("[debug] test ..."), 2, 2, 20, DARKGRAY);
}

void setup_systems_debug(ecs_world_t *world){
        // ONLY 2D
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "render_2d_debug_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
      //.query.terms = {
        //   { .id = ecs_id(Transform3D), .src.id = EcsSelf },
        //   { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot }
      //},
      .callback = render_2d_debug_system
    });
}

void setup_components_debug(ecs_world_t *world){

}

void module_init_debug(ecs_world_t *world){

    setup_components_debug(world);
    setup_systems_debug(world);
}