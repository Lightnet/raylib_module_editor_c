// module_block.c
// for voxel blocks
#include "ecs_components.h"
#include "module_block.h"


void setup_systems_blocks(ecs_world_t *world){

}

void setup_components_blocks(ecs_world_t *world){

}

void module_init_block(ecs_world_t *world){
    setup_components_blocks(world);
    setup_systems_blocks(world);
}