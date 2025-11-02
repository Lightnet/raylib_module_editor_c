#include "raylib.h"
#include "raymath.h"
#include <math.h> // Ensure math.h is included for PI

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MOUSE_SENSITIVITY 0.002f
#define MOVE_SPEED 5.0f
#define SPRINT_MULTIPLIER 2.0f
#define CAMERA_FOV 60.0f
#define CAMERA_MIN_DISTANCE 0.1f
#define MAX_PITCH 89.0f * DEG2RAD

float NormalizeAngle(float angle) {
    // Normalize angle to [-π, π]
    angle = fmod(angle, 2.0f * PI);
    if (angle > PI) {
        angle -= 2.0f * PI;
    } else if (angle < -PI) {
        angle += 2.0f * PI;
    }
    return angle;
}

float NormalizeAngleDegrees(float angle) {
    // Normalize angle to [-180, 180]
    angle = fmod(angle, 360.0f);
    if (angle > 180.0f) {
        angle -= 360.0f;
    } else if (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}

int main(void)
{
    // Initialization
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Raylib FPS Spectator Camera - Mouse Capture");
    SetTargetFPS(60);

    // Capture and hide mouse cursor
    SetMousePosition(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    HideCursor();
    
    // Enable mouse capture
    // SetWindowState(FLAG_WINDOW_MOUSE_LOCKED);
    // EnableCursor();

    // Initialize camera with proper orientation
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 1.8f, 0.0f }; // Eye level
    camera.target = (Vector3){ 0.0f, 1.8f, 1.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = CAMERA_FOV;
    camera.projection = CAMERA_PERSPECTIVE;

    // Camera rotation variables
    float yaw = 0.0f;    // Horizontal rotation
    float pitch = 0.0f;  // Vertical rotation

    Model characterModel = LoadModel("resources/models/block_texture.gltf"); // Load model

    // Simple floor grid for reference
    float floorSize = 100.0f;
    int gridDivisions = 10;

    while (!WindowShouldClose())
    {
        // Get mouse delta (captured mouse movement)
        Vector2 mouseDelta = GetMouseDelta();
        
        // Only update rotation when mouse is captured
        if (IsWindowFocused())
        {
            // Update yaw and pitch from mouse delta
            yaw += mouseDelta.x * MOUSE_SENSITIVITY;
            yaw = NormalizeAngle(yaw);

            // TraceLog(LOG_INFO, "yaw: %f", yaw);

            pitch -= mouseDelta.y * MOUSE_SENSITIVITY;

            // Clamp pitch to prevent flipping
            pitch = Clamp(pitch, -MAX_PITCH, MAX_PITCH);
        }

        // Calculate forward direction using spherical coordinates
        Vector3 forward;
        // yaw = NormalizeAngleDegrees(yaw);
        forward.x = cosf(yaw) * cosf(pitch);
        forward.y = sinf(pitch);
        forward.z = sinf(yaw) * cosf(pitch);
        forward = Vector3Normalize(forward);

        // Update camera target
        camera.target = Vector3Add(camera.position, forward);

        // Movement input
        Vector3 velocity = { 0.0f, 0.0f, 0.0f };
        float speed = MOVE_SPEED;
        
        // Sprint
        if (IsKeyDown(KEY_LEFT_SHIFT))
            speed *= SPRINT_MULTIPLIER;

        // Get normalized forward direction (horizontal)
        Vector3 forwardFlat = forward;
        forwardFlat.y = 0.0f;
        forwardFlat = Vector3Normalize(forwardFlat);

        // Get right vector
        Vector3 right = Vector3CrossProduct(forwardFlat, camera.up);
        right = Vector3Normalize(right);

        // WASD movement
        if (IsKeyDown(KEY_W))
            velocity = Vector3Add(velocity, Vector3Scale(forwardFlat, speed));
        if (IsKeyDown(KEY_S))
            velocity = Vector3Subtract(velocity, Vector3Scale(forwardFlat, speed));
        if (IsKeyDown(KEY_A))
            velocity = Vector3Subtract(velocity, Vector3Scale(right, speed));
        if (IsKeyDown(KEY_D))
            velocity = Vector3Add(velocity, Vector3Scale(right, speed));

        // Vertical movement (spectator mode)
        if (IsKeyDown(KEY_SPACE))
            velocity.y += speed;
        if (IsKeyDown(KEY_LEFT_CONTROL))
            velocity.y -= speed;

        // Apply movement
        Vector3 movement = Vector3Scale(velocity, GetFrameTime());
        camera.position = Vector3Add(camera.position, movement);
        camera.target = Vector3Add(camera.target, movement);

        // Keep cursor centered
        SetMousePosition(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

        // Draw
        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);

        // Draw floor grid
        DrawGrid(floorSize, gridDivisions);

        // Draw reference objects
        // DrawCube((Vector3){-5, 0.5f, -5}, 1.0f, 1.0f, 1.0f, RED);
        // DrawCube((Vector3){5, 0.5f, -5}, 1.0f, 1.0f, 1.0f, GREEN);
        // DrawCube((Vector3){0, 0.5f, -10}, 2.0f, 0.1f, 4.0f, BLUE);
        
        // Draw camera position marker
        DrawSphereWires(camera.position, 0.2f, 8, 8, YELLOW);
        DrawMesh(characterModel.meshes[0], characterModel.materials[1], characterModel.transform);

        EndMode3D();

        // UI
        DrawFPS(10, 10);
        
        DrawText(TextFormat("Yaw: %.1f° Pitch: %.1f°", yaw * RAD2DEG, pitch * RAD2DEG), 10, 30, 20, DARKGRAY);
        DrawText("WASD: Move | Mouse: Look | SHIFT: Sprint | SPACE/CTRL: Fly", 10, 50, 20, DARKGRAY);
        DrawText("ESC: Exit | Mouse captured - Press ALT+F4 to force quit if stuck", 10, 70, 16, RED);

        EndDrawing();
    }

    // Cleanup
    ShowCursor();
    // ClearWindowState(FLAG_WINDOW_MOUSE_LOCKED);
    DisableCursor();
    CloseWindow();

    return 0;
}