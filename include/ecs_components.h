// ecs_components.h
// https://www.flecs.dev/flecs/md_docs_2Quickstart.html
#ifndef ECS_COMPONENTS_H
#define ECS_COMPONENTS_H

#include "raylib.h"
#include "raymath.h"
#include "flecs.h"

// Global phase entities
extern ecs_entity_t PreLogicUpdatePhase;
extern ecs_entity_t LogicUpdatePhase;
extern ecs_entity_t RLBeginDrawingPhase;
extern ecs_entity_t RLRender2D0Phase;
extern ecs_entity_t RLBeginMode3DPhase;
extern ecs_entity_t RLRender3DPhase;
extern ecs_entity_t RLEndMode3DPhase;
extern ecs_entity_t RLRender2D1Phase;
extern ecs_entity_t RLEndDrawingPhase;

// raylib camera 3d
typedef struct {
    Camera3D camera;
} main_context_t;
extern ECS_COMPONENT_DECLARE(main_context_t);

// Transform3D component
typedef struct {
    Vector3 position;         // Local position
    Quaternion rotation;      // Local rotation
    Vector3 scale;            // Local scale
    Matrix localMatrix;       // Local transform matrix
    Matrix worldMatrix;       // World transform matrix
    bool isDirty;             // Flag to indicate if transform needs updating
} Transform3D;
extern ECS_COMPONENT_DECLARE(Transform3D);

// ModelComponent for raylib Model
typedef struct {
    Model* model;             // Pointer to Model
} ModelComponent;
extern ECS_COMPONENT_DECLARE(ModelComponent);

// // PlayerInput_T component
// typedef struct {
//     bool isMovementMode;
//     bool tabPressed;
//     bool moveForward;
//     bool moveBackward;
//     bool moveLeft;
//     bool moveRight;
// } PlayerInput_T;
// extern ECS_COMPONENT_DECLARE(PlayerInput_T);

// // TransformGUI component
// typedef struct {
//     ecs_entity_t id;          // Entity to edit
//     int selectedIndex;        // Index of the selected entity in the list
// } TransformGUI;
// extern ECS_COMPONENT_DECLARE(TransformGUI);


void module_init_raylib(ecs_world_t *world); // Initialization function
void UpdateChildTransformOnly(ecs_world_t *world, ecs_entity_t entity);

#endif // ECS_COMPONENTS_H