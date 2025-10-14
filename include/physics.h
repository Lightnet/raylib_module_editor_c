// physics.h
#ifndef PHYSICS_H
#define PHYSICS_H

#include "ecs_components.h"

void physics_init(ecs_world_t *world);
void physics_system(ecs_iter_t *it);

#endif // PHYSICS_H