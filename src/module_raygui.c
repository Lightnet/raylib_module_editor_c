// module_raygui.c


#include "module_raygui.h"
#include "ecs_components.h" // phase
#include "raygui.h"



void setup_systems_raygui(ecs_world_t *world){

}

void setup_components_raygui(ecs_world_t *world){

}

void module_init_raygui(ecs_world_t *world){
    setup_components_raygui(world);
    setup_systems_raygui(world);
}