#include "raylib.h"
#include "raymath.h" // Added for Vector3 math functions
#include <cglm/cglm.h>
#include <float.h>
#include <math.h>

int main(void) {
    // Initialize window
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "3D Raycast Pick and Place with cglm");
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
    vec3 cubes[GRID_SIZE][GRID_SIZE];
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int z = 0; z < GRID_SIZE; z++) {
            cubes[x][z][0] = (float)x - GRID_SIZE / 2.0f;
            cubes[x][z][1] = 0.5f;
            cubes[x][z][2] = (float)z - GRID_SIZE / 2.0f;
        }
    }

    // Variables for picking and placing
    vec3 selectedCubePos = { 0.0f, -1.0f, 0.0f };
    bool isCubeSelected = false;
    vec3 hoveredCubePos = { 0.0f, -1.0f, 0.0f };
    bool isCubeHovered = false;

    // Disable cursor for mouse look
    DisableCursor();

    // Custom ray structure for cglm
    typedef struct {
        vec3 origin;
        vec3 direction;
    } RayCGLM;

    // Function to convert Raylib mouse position to cglm ray
    void GetMouseRayCGLM(Vector2 mousePos, Camera3D camera, RayCGLM *ray) {
        mat4 view, proj;
        vec4 viewport = { 0.0f, 0.0f, (float)screenWidth, (float)screenHeight };

        vec3 camPos = { camera.position.x, camera.position.y, camera.position.z };
        vec3 camTarget = { camera.target.x, camera.target.y, camera.target.z };
        vec3 camUp = { camera.up.x, camera.up.y, camera.up.z };
        glm_lookat(camPos, camTarget, camUp, view);
        glm_perspective(glm_rad(camera.fovy), (float)screenWidth / screenHeight, 0.1f, 100.0f, proj);

        float nx = (2.0f * mousePos.x) / screenWidth - 1.0f;
        float ny = 1.0f - (2.0f * mousePos.y) / screenHeight;

        vec4 rayClip = { nx, ny, -1.0f, 1.0f };
        vec4 rayEye;
        mat4 invProj, invView;
        glm_mat4_inv(proj, invProj);
        glm_mat4_inv(view, invView);

        glm_mat4_mulv(invProj, rayClip, rayEye);
        rayEye[2] = -1.0f; // Set to -1 for direction
        rayEye[3] = 0.0f;

        vec4 rayWorld;
        glm_mat4_mulv(invView, rayEye, rayWorld);
        glm_vec3_normalize((vec3){rayWorld[0], rayWorld[1], rayWorld[2]});

        glm_vec3_copy(camPos, ray->origin);
        glm_vec3_copy((vec3){rayWorld[0], rayWorld[1], rayWorld[2]}, ray->direction);
    }

    // Ray-AABB intersection test
    bool RayAABBIntersection(RayCGLM ray, vec3 boxMin, vec3 boxMax, float *t) {
        float tMin = 0.0f;
        float tMax = FLT_MAX;

        for (int i = 0; i < 3; i++) {
            if (fabsf(ray.direction[i]) < 1e-6) {
                if (ray.origin[i] < boxMin[i] || ray.origin[i] > boxMax[i]) {
                    return false;
                }
            } else {
                float ood = 1.0f / ray.direction[i];
                float t1 = (boxMin[i] - ray.origin[i]) * ood;
                float t2 = (boxMax[i] - ray.origin[i]) * ood;
                if (t1 > t2) {
                    float temp = t1;
                    t1 = t2;
                    t2 = temp;
                }
                tMin = fmaxf(tMin, t1);
                tMax = fminf(tMax, t2);
                if (tMin > tMax) return false;
            }
        }
        *t = tMin;
        return true;
    }

    // Ray-plane intersection
    bool RayPlaneIntersection(RayCGLM ray, vec3 planeNormal, float planeD, vec3 hitPoint) {
        float denom = glm_vec3_dot(ray.direction, planeNormal); // Fixed: Removed third argument
        if (fabsf(denom) > 1e-6) {
            float t = -(glm_vec3_dot(ray.origin, planeNormal) + planeD) / denom; // Fixed: Removed third argument
            if (t >= 0) {
                glm_vec3_scale(ray.direction, t, hitPoint); // Correct destination type
                glm_vec3_add(hitPoint, ray.origin, hitPoint); // Correct destination type
                return true;
            }
        }
        return false;
    }

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
        RayCGLM ray;
        GetMouseRayCGLM(GetMousePosition(), camera, &ray);
        float closestDist = FLT_MAX;
        isCubeHovered = false;

        for (int x = 0; x < GRID_SIZE; x++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                vec3 boxMin = { cubes[x][z][0] - 0.5f, cubes[x][z][1] - 0.5f, cubes[x][z][2] - 0.5f };
                vec3 boxMax = { cubes[x][z][0] + 0.5f, cubes[x][z][1] + 0.5f, cubes[x][z][2] + 0.5f };
                float t;
                if (RayAABBIntersection(ray, boxMin, boxMax, &t) && t < closestDist) {
                    closestDist = t;
                    glm_vec3_copy(cubes[x][z], hoveredCubePos);
                    isCubeHovered = true;
                }
            }
        }

        // Pick cube on left click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isCubeHovered) {
            float minDist = FLT_MAX;
            vec3 tempSelectedPos = { 0.0f, -1.0f, 0.0f };
            for (int x = 0; x < GRID_SIZE; x++) {
                for (int z = 0; z < GRID_SIZE; z++) {
                    vec3 boxMin = { cubes[x][z][0] - 0.5f, cubes[x][z][1] - 0.5f, cubes[x][z][2] - 0.5f };
                    vec3 boxMax = { cubes[x][z][0] + 0.5f, cubes[x][z][1] + 0.5f, cubes[x][z][2] + 0.5f };
                    float t;
                    if (RayAABBIntersection(ray, boxMin, boxMax, &t) && t < minDist) {
                        minDist = t;
                        glm_vec3_copy(cubes[x][z], tempSelectedPos);
                    }
                }
            }
            if (minDist != FLT_MAX) {
                glm_vec3_copy(tempSelectedPos, selectedCubePos);
                isCubeSelected = true;
            }
        }

        // --- Raycasting for Placing ---
        if (isCubeSelected && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            vec3 planeNormal = { 0.0f, 1.0f, 0.0f };
            float planeD = 0.0f;
            vec3 hitPoint;
            if (RayPlaneIntersection(ray, planeNormal, planeD, hitPoint)) {
                float newX = roundf(hitPoint[0]);
                float newZ = roundf(hitPoint[2]);
                newX = Clamp(newX, -GRID_SIZE / 2.0f, GRID_SIZE / 2.0f - 1.0f);
                newZ = Clamp(newZ, -GRID_SIZE / 2.0f, GRID_SIZE / 2.0f - 1.0f);
                vec3 newPos = { newX, 0.5f, newZ };

                bool isOccupied = false;
                for (int x = 0; x < GRID_SIZE; x++) {
                    for (int z = 0; z < GRID_SIZE; z++) {
                        float dist = glm_vec3_distance(cubes[x][z], newPos); // Fixed: Removed third argument
                        float selDist = glm_vec3_distance(cubes[x][z], selectedCubePos); // Fixed
                        if (dist < 0.1f && selDist > 0.1f) {
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
                            float dist = glm_vec3_distance(cubes[x][z], selectedCubePos); // Fixed
                            if (dist < minDist) {
                                minDist = dist;
                                selectedX = x;
                                selectedZ = z;
                            }
                        }
                    }
                    glm_vec3_copy(newPos, cubes[selectedX][selectedZ]);
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
                Vector3 cubePos = { cubes[x][z][0], cubes[x][z][1], cubes[x][z][2] };
                float dist = glm_vec3_distance(cubes[x][z], selectedCubePos); // Fixed
                Color color = (dist < 0.1f && isCubeSelected) ? RED : BLUE;
                DrawCube(cubePos, 1.0f, 1.0f, 1.0f, color);
                float distHover = glm_vec3_distance(cubes[x][z], hoveredCubePos); // Fixed
                if (isCubeHovered && distHover < 0.1f) {
                    DrawCubeWires(cubePos, 1.0f, 1.0f, 1.0f, YELLOW);
                } else {
                    DrawCubeWires(cubePos, 1.0f, 1.0f, 1.0f, BLACK);
                }
            }
        }

        Ray raylibRay = { 
            { ray.origin[0], ray.origin[1], ray.origin[2] },
            { ray.direction[0], ray.direction[1], ray.direction[2] }
        };
        DrawRay(raylibRay, GREEN);

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