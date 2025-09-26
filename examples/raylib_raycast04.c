// 

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
    Vector3 hoveredCubePos = { 0.0f, -1.0f, 0.0f }; // For highlighting cube under mouse
    bool isCubeHovered = false;

    // Disable cursor for mouse look
    DisableCursor();

    while (!WindowShouldClose()) {
        // --- Camera Movement ---
        Vector3 move = { 0.0f, 0.0f, 0.0f };
        float moveSpeed = 0.1f;

        if (IsKeyDown(KEY_W)) move.x += moveSpeed;
        if (IsKeyDown(KEY_S)) move.x -= moveSpeed;
        if (IsKeyDown(KEY_A)) move.z -= moveSpeed;
        if (IsKeyDown(KEY_D)) move.z += moveSpeed;
        if (IsKeyDown(KEY_SPACE)) move.y += moveSpeed;
        if (IsKeyDown(KEY_LEFT_SHIFT)) move.y -= moveSpeed;

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

        Vector3 direction = Vector3Subtract(camera.target, camera.position);
        float distance = Vector3Length(direction);
        direction = Vector3Normalize(direction);

        direction = Vector3RotateByAxisAngle(direction, (Vector3){ 0.0f, 1.0f, 0.0f }, -mouseX);
        float pitch = asinf(direction.y) - mouseY;
        pitch = Clamp(pitch, -PI / 2.0f + 0.1f, PI / 2.0f - 0.1f);
        float yawAngle = atan2f(direction.z, direction.x);
        direction = (Vector3){
            cosf(pitch) * cosf(yawAngle),
            sinf(pitch),
            cosf(pitch) * sinf(yawAngle)
        };
        camera.target = Vector3Add(camera.position, Vector3Scale(direction, distance));

        // --- Raycasting for Picking and Hover ---
        Ray ray = GetMouseRay(GetMousePosition(), camera);
        float closestDist = FLT_MAX;
        isCubeHovered = false;

        for (int x = 0; x < GRID_SIZE; x++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                BoundingBox box = {
                    (Vector3){ cubes[x][z].x - 0.5f, cubes[x][z].y - 0.5f, cubes[x][z].z - 0.5f },
                    (Vector3){ cubes[x][z].x + 0.5f, cubes[x][z].y + 0.5f, cubes[x][z].z + 0.5f }
                };
                RayCollision collision = GetRayCollisionBox(ray, box);
                if (collision.hit && collision.distance < closestDist) {
                    closestDist = collision.distance;
                    hoveredCubePos = cubes[x][z];
                    isCubeHovered = true;
                }
            }
        }

        // Pick cube on left click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isCubeHovered) {
            float minDist = FLT_MAX;
            Vector3 tempSelectedPos = { 0.0f, -1.0f, 0.0f };
            for (int x = 0; x < GRID_SIZE; x++) {
                for (int z = 0; z < GRID_SIZE; z++) {
                    BoundingBox box = {
                        (Vector3){ cubes[x][z].x - 0.5f, cubes[x][z].y - 0.5f, cubes[x][z].z - 0.5f },
                        (Vector3){ cubes[x][z].x + 0.5f, cubes[x][z].y + 0.5f, cubes[x][z].z + 0.5f }
                    };
                    RayCollision collision = GetRayCollisionBox(ray, box);
                    if (collision.hit && collision.distance < minDist) {
                        minDist = collision.distance;
                        tempSelectedPos = cubes[x][z];
                    }
                }
            }
            if (minDist != FLT_MAX) {
                selectedCubePos = tempSelectedPos;
                isCubeSelected = true;
            }
        }

        // --- Raycasting for Placing ---
        if (isCubeSelected && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            RayCollision collision = GetRayCollisionQuad(ray,
                (Vector3){ -GRID_SIZE / 2.0f, 0.0f, -GRID_SIZE / 2.0f },
                (Vector3){ GRID_SIZE / 2.0f, 0.0f, -GRID_SIZE / 2.0f },
                (Vector3){ GRID_SIZE / 2.0f, 0.0f, GRID_SIZE / 2.0f },
                (Vector3){ -GRID_SIZE / 2.0f, 0.0f, GRID_SIZE / 2.0f });

            if (collision.hit) {
                float newX = roundf(collision.point.x);
                float newZ = roundf(collision.point.z);
                newX = Clamp(newX, -GRID_SIZE / 2.0f, GRID_SIZE / 2.0f - 1.0f);
                newZ = Clamp(newZ, -GRID_SIZE / 2.0f, GRID_SIZE / 2.0f - 1.0f);
                Vector3 newPos = { newX, 0.5f, newZ };

                bool isOccupied = false;
                for (int x = 0; x < GRID_SIZE; x++) {
                    for (int z = 0; z < GRID_SIZE; z++) {
                        if (Vector3Distance(cubes[x][z], newPos) < 0.1f &&
                            Vector3Distance(cubes[x][z], selectedCubePos) > 0.1f) {
                            isOccupied = true;
                            break;
                        }
                    }
                }

                if (!isOccupied) {
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
        }

        // --- Drawing ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawGrid(GRID_SIZE, 1.0f);

        for (int x = 0; x < GRID_SIZE; x++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                Color color = (Vector3Distance(cubes[x][z], selectedCubePos) < 0.1f && isCubeSelected) ? RED : BLUE;
                DrawCube(cubes[x][z], 1.0f, 1.0f, 1.0f, color);
                if (isCubeHovered && Vector3Distance(cubes[x][z], hoveredCubePos) < 0.1f) {
                    DrawCubeWires(cubes[x][z], 1.0f, 1.0f, 1.0f, YELLOW);
                } else {
                    DrawCubeWires(cubes[x][z], 1.0f, 1.0f, 1.0f, BLACK);
                }
            }
        }

        DrawRay(ray, GREEN);

        EndMode3D();

        DrawText("WASD: Move, SPACE/Shift: Up/Down, Mouse: Look", 10, 10, 20, DARKGRAY);
        DrawText("Left Click: Pick Cube, Right Click: Place Cube", 10, 40, 20, DARKGRAY);
        DrawText("Yellow Wireframe: Hovered Cube, Green Line: Raycast", 10, 70, 20, DARKGRAY);
        DrawText("x", (int)screenWidth / 2, (int)screenHeight / 2, 10, DARKGRAY);

        EndDrawing();
    }


    CloseWindow();
    return 0;
}