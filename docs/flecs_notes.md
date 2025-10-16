# design:

# flecs notes:

```
ecs_entity_t my_entity = ecs_new(world);
```
 By default it create from number 537+ max is 4 billion. Can be found in flecs.h line number 1154.


# world:

```c
// ecs world
ecs_world_t *world = ecs_init();
//...
//set up

// loop
    //...
    ecs_progress(world, 0);//update ecs
    //..
// end loop
// De-Initialization
ecs_fini(world);

```
# entity:

```c
ecs_entity_t node1 = ecs_entity(world, {
    .name = "NodeParent"
});
```

```c
ecs_entity_t node2 = ecs_entity(world, {
    .name = "NodeChild",
    .parent = node1
});
```



# format:
- Field index 0 (after implicit 'this' entity)
```c
// Component definition: Matches DrawText params
typedef struct {
    const char* text;
    int x;
    int y;
    int fontSize;
    Color color;
} Text;
//...
void RenderText(ecs_iter_t *it) {
    Text *texts = ecs_field(it, Text, 0);  // Field index 0 (after implicit 'this' entity)

    for (int i = 0; i < it->count; i++) {
        Text *t = &texts[i];
        DrawText(t->text, t->x, t->y, t->fontSize, t->color);
    }
}
//...
ECS_SYSTEM(world, RenderText, RLPHASE_RENDER, Text);
//...
// Create entity and add component with params from DrawText
// (x=190, y=200, fontSize=20; text and color as literals)
ecs_entity_t uiText = ecs_new(world);
ecs_set_name(world, uiText, "ui_text");  // Optional: Name for debugging
ecs_set(world, uiText, Text, {
    .text = "Congrats! You created your first window!",
    .x = 190,
    .y = 200,
    .fontSize = 20,
    // .color = LIGHTGRAY  // Raylib predefined color
    .color = RED  // Raylib predefined color
});



```
- 

# phase tag:
```c
// Phases must have the EcsPhase tag
ecs_entity_t RLPHASE_BEGINDRAW = ecs_new_w_id(world, EcsPhase);
ecs_entity_t RLPHASE_RENDER = ecs_new_w_id(world, EcsPhase);
ecs_entity_t RLPHASE_ENDRAW = ecs_new_w_id(world, EcsPhase);
```

# phase update:
```c
ecs_add_pair(world, RLPHASE_BEGINDRAW, EcsDependsOn, EcsOnUpdate);
ecs_add_pair(world, RLPHASE_RENDER, EcsDependsOn, RLPHASE_BEGINDRAW);
ecs_add_pair(world, RLPHASE_ENDRAW, EcsDependsOn, RLPHASE_RENDER);
```

# system update:

```c
void RL_Begin_Draw_System(ecs_iter_t *it){
    printf("draw\n");
    //...
}

ecs_system(world, {
    .entity = ecs_entity(world, {
        .name = "RLBeginDrawingSystem",
        .add = ecs_ids( ecs_dependson(RLPHASE_BEGINDRAW) )
    }),
    .callback = RL_Begin_Draw_System,
});
```

# entity:
  There couple way to handle entity set up.

```c
// Component definition: Matches DrawText params
typedef struct {
    const char* text;
    int x;
    int y;
    int fontSize;
    Color color;
} Text;

ECS_COMPONENT(world, Text);


// Create entity and add component with params from DrawText
// (x=190, y=200, fontSize=20; text and color as literals)
ecs_entity_t uiText = ecs_new(world);
ecs_set_name(world, uiText, "ui_text");  // Optional: set Name for entity
ecs_set(world, uiText, Text, {
    .text = "Congrats! You created your first window!",
    .x = 190,
    .y = 200,
    .fontSize = 20,
    // .color = LIGHTGRAY  // Raylib predefined color
    .color = RED  // Raylib predefined color
});
```


# Transform:


```c
// Transform3D component
typedef struct {
    Vector3 position;         // Local position
    Quaternion rotation;      // Local rotation
    Vector3 scale;            // Local scale
    Matrix localMatrix;       // Local transform matrix
    Matrix worldMatrix;       // World transform matrix
    bool isDirty;             // Flag to indicate if transform needs updating
} Transform3D;
ECS_COMPONENT_DECLARE(Transform3D);
```

```c
Transform3D.position //for user to move object
Transform3D.rotation //for user to rotation object
```


```c
//...
m[i].model->transform = t[i].worldMatrix;
bool isChild = ecs_has_pair(it->world, entity, EcsChildOf, EcsWildcard);
DrawModel(*(m[i].model), (Vector3){0, 0, 0}, 1.0f, isChild ? BLUE : RED);
//...
```







```c
ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, {
        .name = "UpdateTransformHierarchySystem",
        .add = ecs_ids(ecs_dependson(PreLogicUpdatePhase))
    }),
    .query.terms = {
        { .id = ecs_id(Transform3D), .src.id = EcsSelf },
        { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot } // No parent
    },
    .callback = UpdateTransformHierarchySystem
});
```

```c
// System to update all transforms in hierarchical order
void UpdateTransformHierarchySystem(ecs_iter_t *it) {
  // Process only root entities (no parent)
  Transform3D *transforms = ecs_field(it, Transform3D, 0);
  for (int i = 0; i < it->count; i++) {
      ecs_entity_t entity = it->entities[i];
      ProcessEntityHierarchy(it->world, entity);
  }
}
```
```c
// Recursive function to process entity and its children
void ProcessEntityHierarchy(ecs_world_t *world, ecs_entity_t entity) {
  // Update the current entity's transform
  Transform3D *transform = ecs_get_mut(world, entity, Transform3D);
  bool wasUpdated = false;
  if (transform) {
      // Only update if dirty or parent is dirty
      ecs_entity_t parent = ecs_get_parent(world, entity);
      bool parentIsDirty = false;
      if (parent && ecs_is_valid(world, parent)) {
          const Transform3D *parent_transform = ecs_get(world, parent, Transform3D);
          if (parent_transform && parent_transform->isDirty) {
              parentIsDirty = true;
          }
      }
      if (transform->isDirty || parentIsDirty) {
          UpdateTransform(world, entity, transform);
          //ecs_set(world, entity, Transform3D, *transform); // Commit changes
          wasUpdated = true;
      }
  }

  // Skip processing children if this entity was not updated
  if (!wasUpdated) {
      return;
  }

  // Iterate through children
  ecs_iter_t it = ecs_children(world, entity);
  while (ecs_children_next(&it)) {
      for (int i = 0; i < it.count; i++) {
          ecs_entity_t child = it.entities[i];
          ProcessEntityHierarchy(world, child); // Recursively process child
      }
  }
}
```

```c
// Helper function to update a single transform
void UpdateTransform(ecs_world_t *world, ecs_entity_t entity, Transform3D *transform) {
  // Get parent entity
  ecs_entity_t parent = ecs_get_parent(world, entity);
  const char *name = ecs_get_name(world, entity) ? ecs_get_name(world, entity) : "(unnamed)";
  bool parentIsDirty = false;

  // Check if parent is dirty
  if (parent && ecs_is_valid(world, parent)) {
      const Transform3D *parent_transform = ecs_get(world, parent, Transform3D);
      if (parent_transform && parent_transform->isDirty) {
          parentIsDirty = true;
      }
  }

  // Skip update if neither this transform nor its parent is dirty
  if (!transform->isDirty && !parentIsDirty) {
      // printf("Skipping update for %s (not dirty)\n", name);
      return;
  }

  // Compute local transform
  Matrix translation = MatrixTranslate(transform->position.x, transform->position.y, transform->position.z);
  Matrix rotation = QuaternionToMatrix(transform->rotation);
  Matrix scaling = MatrixScale(transform->scale.x, transform->scale.y, transform->scale.z);
  transform->localMatrix = MatrixMultiply(scaling, MatrixMultiply(rotation, translation));

  if (!parent || !ecs_is_valid(world, parent)) {
      // Root entity: world matrix = local matrix
      transform->worldMatrix = transform->localMatrix;
    //   printf("Root %s position (%.2f, %.2f, %.2f)\n", name, transform->position.x, transform->position.y, transform->position.z);
  } else {
      // Child entity: world matrix = local matrix * parent world matrix
      const Transform3D *parent_transform = ecs_get(world, parent, Transform3D);
      if (!parent_transform) {
        //   printf("Error: Parent %s lacks Transform3D for %s\n",
        //          ecs_get_name(world, parent) ? ecs_get_name(world, parent) : "(unnamed)", name);
          transform->worldMatrix = transform->localMatrix;
          return;
      }

      // Validate parent world matrix
      float px = parent_transform->worldMatrix.m12;
      float py = parent_transform->worldMatrix.m13;
      float pz = parent_transform->worldMatrix.m14;
      if (fabs(px) > 1e6 || fabs(py) > 1e6 || fabs(pz) > 1e6) {
        //   printf("Error: Invalid parent %s world pos (%.2f, %.2f, %.2f) for %s\n",
        //          ecs_get_name(world, parent) ? ecs_get_name(world, parent) : "(unnamed)",
        //          px, py, pz, name);
          transform->worldMatrix = transform->localMatrix;
          return;
      }

      // Compute world matrix
      transform->worldMatrix = MatrixMultiply(transform->localMatrix, parent_transform->worldMatrix);

      // Extract world position
      float wx = transform->worldMatrix.m12;
      float wy = transform->worldMatrix.m13;
      float wz = transform->worldMatrix.m14;

      // Debug output
    //   const char *parent_name = ecs_get_name(world, parent) ? ecs_get_name(world, parent) : "(unnamed)";
    //   printf("Child %s (ID: %llu), parent %s (ID: %llu)\n",
    //          name, (unsigned long long)entity, parent_name, (unsigned long long)parent);
    //   printf("Child %s position (%.2f, %.2f, %.2f), parent %s world pos (%.2f, %.2f, %.2f), world pos (%.2f, %.2f, %.2f)\n",
    //          name, transform->position.x, transform->position.y, transform->position.z,
    //          parent_name, px, py, pz, wx, wy, wz);
  }

  // Mark children as dirty to ensure they update in the next frame
  ecs_iter_t it = ecs_children(world, entity);
  while (ecs_children_next(&it)) {
      for (int i = 0; i < it.count; i++) {
          Transform3D *child_transform = ecs_get_mut(world, it.entities[i], Transform3D);
          if (child_transform) {
              child_transform->isDirty = true;
              //ecs_set(world, it.entities[i], Transform3D, *child_transform);
          }
      }
  }

  // Reset isDirty after updating
  transform->isDirty = false;
}
```


```c
ecs_entity_t node1 = ecs_entity(world, {
    .name = "NodeParent"
});

ecs_set(world, node1, Transform3D, {
    .position = (Vector3){0.0f, 0.0f, 0.0f},
    .rotation = QuaternionIdentity(),
    .scale = (Vector3){1.0f, 1.0f, 1.0f},
    .localMatrix = MatrixIdentity(),
    .worldMatrix = MatrixIdentity(),
    .isDirty = true
});

ecs_entity_t node2 = ecs_entity(world, {
    .name = "NodeChild",
    .parent = node1
});
ecs_set(world, node2, Transform3D, {
    .position = (Vector3){2.0f, 0.0f, 0.0f},
    .rotation = QuaternionIdentity(),
    .scale = (Vector3){0.5f, 0.5f, 0.5f},
    .localMatrix = MatrixIdentity(),
    .worldMatrix = MatrixIdentity(),
    .isDirty = true
});
```

- https://www.flecs.dev/flecs/md_docs_2Quickstart.html

```c
// Module header (e.g. MyModule.h)
typedef struct {
    float x;
    float y;
} Position;
 
extern ECS_COMPONENT_DECLARE(Position);
 
// The import function name has to follow the convention: <ModuleName>Import
void MyModuleImport(ecs_world_t *world);
```

```c
// Module source (e.g. MyModule.c)
ECS_COMPONENT_DECLARE(Position);
 
void MyModuleImport(ecs_world_t *world) {
    ECS_MODULE(world, MyModule);
    ECS_COMPONENT_DEFINE(world, Position);
}
 
// Import code
ECS_IMPORT(world, MyModule);
```