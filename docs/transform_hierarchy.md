

- examples/raylib3d_flecs_transform_hierarchy09_gui.c # clean up to simple.

# Transform hierarchy:
  Kept it simple. As using the Grok AI to design to be simple to handle transform hierarchy.

  Since using the flecs system loop update required different way to handle parent and childrens.

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
    .parent = node1 //flces default parenting
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


