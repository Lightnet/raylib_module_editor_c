#ifndef MODULE_ENET_H
#define MODULE_ENET_H

#include "flecs.h"

// TransformGUI component
typedef struct {
    void *data;
} enet_packet_t;
extern ECS_COMPONENT_DECLARE(enet_packet_t);

// Declare event entity as extern for global access
extern ecs_entity_t event_receive_packed;

void module_init_enet(ecs_world_t *world);

#endif // MODULE_ENET_H