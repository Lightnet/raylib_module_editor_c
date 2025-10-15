#pragma once

#include "flecs.h"
#include "ode/ode.h"


// Components
typedef struct {
    dBodyID id;
} ode_body_t;
extern ECS_COMPONENT_DECLARE(ode_body_t);

typedef struct {
    dGeomID id;
} ode_geom_t;
extern ECS_COMPONENT_DECLARE(ode_geom_t);

typedef struct {
    dWorldID world;
    dSpaceID space;
    dJointGroupID contact_group;
} ode_context_t;
extern ECS_COMPONENT_DECLARE(ode_context_t);


void module_init_ode(ecs_world_t *world); // Initialization function