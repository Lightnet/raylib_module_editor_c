/*********************************************************************
 *  raylib 5.5 – Third-Person 45° View
 *  Player moves forward in camera direction + 3D reference point
 *********************************************************************/

#include "raylib.h"
#include "raymath.h"

int main(void)
{
    // --------------------------------------------------------------
    // Window
    // --------------------------------------------------------------
    const int screenWidth  = 1024;
    const int screenHeight =  720;
    InitWindow(screenWidth, screenHeight,
               "raylib 5.5 – Third-Person 45° + Forward Ref Point");
    SetTargetFPS(60);

    // --------------------------------------------------------------
    // Player
    // --------------------------------------------------------------
    Vector3 playerPos = { 0.0f, 0.5f, 0.0f };
    const float playerSize = 1.0f;
    const float moveSpeed  = 0.12f;

    // --------------------------------------------------------------
    // Camera parameters
    // --------------------------------------------------------------
    const float camDistance = 8.0f;
    const float camPitch    = 45.0f * DEG2RAD;
    float camYaw = 0.0f;

    Camera3D camera = { 0 };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    // --------------------------------------------------------------
    // Mouse for yaw
    // --------------------------------------------------------------
    Vector2 mousePrev = { 0 };
    bool mousePressed = false;

    // --------------------------------------------------------------
    // Reference point settings
    // --------------------------------------------------------------
    const float refDistance = 10.0f;  // 3 units in front of player
    const float refSize     = 0.3f;
    const Color refColor    = YELLOW;

    // --------------------------------------------------------------
    // Main loop
    // --------------------------------------------------------------
    while (!WindowShouldClose())
    {
        // ----------------------------------------------------------
        // 1. Update yaw with mouse (LMB drag)
        // ----------------------------------------------------------
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            mousePrev    = GetMousePosition();
            mousePressed = true;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) mousePressed = false;

        if (mousePressed)
        {
            Vector2 mouseNow = GetMousePosition();
            Vector2 delta    = Vector2Subtract(mouseNow, mousePrev);
            camYaw -= delta.x * 0.005f;
            mousePrev = mouseNow;
        }

        // ----------------------------------------------------------
        // 2. Update camera position (45° down, behind player)
        // ----------------------------------------------------------
        Vector3 camOffset = {
            camDistance * cosf(camPitch) * sinf(camYaw),
            camDistance * sinf(camPitch),
            camDistance * cosf(camPitch) * cosf(camYaw)
        };
        camera.position = Vector3Add(playerPos, camOffset);
        camera.target   = playerPos;

        // ----------------------------------------------------------
        // 3. Compute forward direction (XZ plane)
        // ----------------------------------------------------------
        Vector3 camForward = Vector3Subtract(camera.target, camera.position);
        camForward = Vector3Normalize(camForward);
        Vector3 forwardXZ = { camForward.x, 0.0f, camForward.z };
        forwardXZ = Vector3Normalize(forwardXZ);

        Vector3 rightXZ = Vector3CrossProduct(forwardXZ, (Vector3){0,1,0});
        rightXZ = Vector3Normalize(rightXZ);

        // ----------------------------------------------------------
        // 4. Compute reference point (in front of player)
        // ----------------------------------------------------------
        Vector3 refPoint = Vector3Add(playerPos, Vector3Scale(forwardXZ, refDistance));

        // ----------------------------------------------------------
        // 5. Player movement (WASD relative to camera)
        // ----------------------------------------------------------
        Vector3 move = { 0 };
        if (IsKeyDown(KEY_W)) move = Vector3Add(move, Vector3Scale(forwardXZ, moveSpeed));
        if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, Vector3Scale(forwardXZ, moveSpeed));
        if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, Vector3Scale(rightXZ,   moveSpeed));
        if (IsKeyDown(KEY_D)) move = Vector3Add(move,    Vector3Scale(rightXZ,   moveSpeed));

        playerPos = Vector3Add(playerPos, move);

        // ----------------------------------------------------------
        // 6. Draw
        // ----------------------------------------------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // Grid
        DrawGrid(40, 1.0f);

        // Player cube
        DrawCube(playerPos, playerSize, playerSize, playerSize, RED);
        DrawCubeWires(playerPos, playerSize, playerSize, playerSize, MAROON);

        // Reference point (yellow sphere)
        DrawSphere(refPoint, refSize, refColor);
        DrawSphereWires(refPoint, refSize, 8, 8, DARKGRAY);

        // Optional: line from player to ref point
        DrawLine3D(playerPos, refPoint, RED);

        // Origin
        DrawSphere((Vector3){0,0.1f,0}, 0.2f, GREEN);

        EndMode3D();

        // 2D overlay
        DrawText("WASD = move (W = toward yellow point)", 10, 10, 20, DARKGRAY);
        DrawText("LMB drag = rotate camera", 10, 35, 20, DARKGRAY);
        DrawFPS(10, 60);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}