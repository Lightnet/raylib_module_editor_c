#include "raylib.h"
#include "raymath.h"
#include <float.h>

int main(void) {
    // Initialize window
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "3D Raycast Pick and Place");
    SetTargetFPS(60);

    // Set up camera
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Define a grid of cubes
    #define GRID_SIZE 5
    Vector3 cubes[GRID_SIZE][GRID_SIZE];
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int z = 0; z < GRID_SIZE; z++) {
            cubes[x][z] = (Vector3){ (float)x - GRID_SIZE / 2.0f, 0.5f, (float)z - GRID_SIZE / 2.0f };
        }
    }

    // Variables for picking and placing
    Vector3 selectedCubePos = { 0.0f, -1.0f, 0.0f }; // Initially no cube selected
    bool isCubeSelected = false;

    // Disable cursor for mouse look
    DisableCursor();

    while (!WindowShouldClose()) {
        // --- Camera Movement ---
        Vector3 move = { 0.0f, 0.0f, 0.0f };
        float moveSpeed = 0.1f;

        // if (IsKeyDown(KEY_W)) move.z += moveSpeed;
        // if (IsKeyDown(KEY_S)) move.z -= moveSpeed;
        // if (IsKeyDown(KEY_A)) move.x -= moveSpeed;
        // if (IsKeyDown(KEY_D)) move.x += moveSpeed;

        if (IsKeyDown(KEY_W)) move.x += moveSpeed;
        if (IsKeyDown(KEY_S)) move.x -= moveSpeed;
        if (IsKeyDown(KEY_A)) move.z -= moveSpeed;
        if (IsKeyDown(KEY_D)) move.z += moveSpeed;
        if (IsKeyDown(KEY_SPACE)) move.y += moveSpeed;
        if (IsKeyDown(KEY_LEFT_SHIFT)) move.y -= moveSpeed;

        // Rotate movement vector based on camera yaw
        float yaw = atan2f(camera.target.z - camera.position.z, camera.target.x - camera.position.x);
        Vector3 rotatedMove = {
            move.x * cosf(yaw) - move.z * sinf(yaw),
            move.y,
            move.x * sinf(yaw) + move.z * cosf(yaw)
        };
        camera.position = Vector3Add(camera.position, rotatedMove);
        camera.target = Vector3Add(camera.target, rotatedMove);

        // --- Camera Rotation ---
        float mouseSensitivity = 0.005f;
        float mouseX = GetMouseDelta().x * mouseSensitivity;
        float mouseY = GetMouseDelta().y * mouseSensitivity;

        // Calculate direction vector
        Vector3 direction = Vector3Subtract(camera.target, camera.position);
        float distance = Vector3Length(direction);
        direction = Vector3Normalize(direction);

        // Apply yaw (horizontal rotation)
        direction = Vector3RotateByAxisAngle(direction, (Vector3){ 0.0f, 1.0f, 0.0f }, -mouseX);

        // Apply pitch (vertical rotation) with limits
        float pitch = asinf(direction.y) - mouseY;
        pitch = Clamp(pitch, -PI / 2.0f + 0.1f, PI / 2.0f - 0.1f); // Limit to avoid flipping
        float yawAngle = atan2f(direction.z, direction.x);
        direction = (Vector3){
            cosf(pitch) * cosf(yawAngle),
            sinf(pitch),
            cosf(pitch) * sinf(yawAngle)
        };

        // Update camera target
        camera.target = Vector3Add(camera.position, Vector3Scale(direction, distance));

        // --- Raycasting for Picking ---
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Ray ray = GetMouseRay(GetMousePosition(), camera);
            float closestDist = FLT_MAX;
            isCubeSelected = false;

            // Check for cube intersection
            for (int x = 0; x < GRID_SIZE; x++) {
                for (int z = 0; z < GRID_SIZE; z++) {
                    BoundingBox box = {
                        (Vector3){ cubes[x][z].x - 0.5f, cubes[x][z].y - 0.5f, cubes[x][z].z - 0.5f },
                        (Vector3){ cubes[x][z].x + 0.5f, cubes[x][z].y + 0.5f, cubes[x][z].z + 0.5f }
                    };
                    RayCollision collision = GetRayCollisionBox(ray, box);
                    if (collision.hit && collision.distance < closestDist) {
                        closestDist = collision.distance;
                        selectedCubePos = cubes[x][z];
                        isCubeSelected = true;
                    }
                }
            }
        }

        // --- Raycasting for Placing ---
        if (isCubeSelected && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            Ray ray = GetMouseRay(GetMousePosition(), camera);
            RayCollision collision = GetRayCollisionQuad(ray,
                (Vector3){ -GRID_SIZE / 2.0f, 0.0f, -GRID_SIZE / 2.0f },
                (Vector3){ GRID_SIZE / 2.0f, 0.0f, -GRID_SIZE / 2.0f },
                (Vector3){ GRID_SIZE / 2.0f, 0.0f, GRID_SIZE / 2.0f },
                (Vector3){ -GRID_SIZE / 2.0f, 0.0f, GRID_SIZE / 2.0f });

            if (collision.hit) {
                // Snap to grid
                Vector3 newPos = {
                    roundf(collision.point.x),
                    0.5f,
                    roundf(collision.point.z)
                };
                // Update the closest cube's position
                float minDist = FLT_MAX;
                int selectedX = 0, selectedZ = 0;
                for (int x = 0; x < GRID_SIZE; x++) {
                    for (int z = 0; z < GRID_SIZE; z++) {
                        float dist = Vector3Distance(cubes[x][z], selectedCubePos);
                        if (dist < minDist) {
                            minDist = dist;
                            selectedX = x;
                            selectedZ = z;
                        }
                    }
                }
                cubes[selectedX][selectedZ] = newPos;
                isCubeSelected = false;
            }
        }

        // --- Drawing ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        // Draw ground plane
        DrawGrid(GRID_SIZE, 1.0f);

        // Draw cubes
        for (int x = 0; x < GRID_SIZE; x++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                Color color = (Vector3Distance(cubes[x][z], selectedCubePos) < 0.1f && isCubeSelected) ? RED : BLUE;
                DrawCube(cubes[x][z], 1.0f, 1.0f, 1.0f, color);
                DrawCubeWires(cubes[x][z], 1.0f, 1.0f, 1.0f, BLACK);
            }
        }
        EndMode3D();

        // Draw instructions
        DrawText("WASD: Move, SPACE/Shift: Up/Down, Mouse: Look", 10, 10, 20, DARKGRAY);
        DrawText("Left Click: Pick Cube, Right Click: Place Cube", 10, 40, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}