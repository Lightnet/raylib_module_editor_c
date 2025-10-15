// physics 3D
// https://github.com/thomasmarsh/ODE

#include "ecs_components.h" // phase
#include "module_ode.h"
#include "raygui.h"


// Declare and define component in the source file
ECS_COMPONENT_DECLARE(ode_body_t);
ECS_COMPONENT_DECLARE(ode_geom_t);
ECS_COMPONENT_DECLARE(ode_context_t);

// Callback for collision detection
static void nearCallback(void *data, dGeomID o1, dGeomID o2) {
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    if (b1 && b2 && dAreConnected(b1, b2)) return;

    dContact contact;
    contact.surface.mode = dContactBounce;
    contact.surface.mu = dInfinity; // Friction
    contact.surface.bounce = 0.5f;   // Bounciness
    contact.surface.bounce_vel = 0.1f;
    contact.surface.soft_cfm = 0.01f;

    // if (dCollide(o1, o2, 1, &contact.geom, sizeof(dContact))) {
    //     // dJointID c = dJointCreateContact(world, contact_group, &contact);
    //     dJointAttach(c, b1, b2);
    // }
}


void setup_systems_ode(ecs_world_t *world){
    ECS_COMPONENT_DEFINE(world, ode_body_t);
    ECS_COMPONENT_DEFINE(world, ode_geom_t);
    ECS_COMPONENT_DEFINE(world, ode_context_t);

}


void setup_components_ode(ecs_world_t *world){


}



void module_init_edo(ecs_world_t *world){

    setup_components_ode(world);

    setup_systems_ode();
}