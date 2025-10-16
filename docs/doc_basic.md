# Informtion:
  This project will try to make module in some ways.

  Working toward 3D module which required a lot of work.

  There are 3D and 2D set up for phases system call loop.

# Phases:
```c
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
```


# Setup Camera 3D:
  Work in progress.
```c
    // setup Camera 3D
    Camera3D camera = {
        .position = (Vector3){10.0f, 10.0f, 10.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };
    ecs_singleton_set(world, main_context_t, {
        .camera = camera
    });
```
  This is main camera render. 