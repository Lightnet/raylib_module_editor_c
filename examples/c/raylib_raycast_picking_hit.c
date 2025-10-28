#include "raylib.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - 3d camera mode");

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 10.0f, 10.0f };  // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type

    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };
    Vector3 cubeSize = { 1.0f, 1.0f, 1.0f };

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    Ray ray = { 0 };                // Picking line ray
    RayCollision collision = { 0 }; // Ray collision hit info

    // Initialize ray
    ray.position = (Vector3){ 0.0f, 3.0f, 0.0f };  // Corrected duplicate x assignment
    ray.direction = (Vector3){ 0.0f, -1.0f, 0.0f }; // Corrected duplicate x assignment

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // Movement controls for ray position (W, S, A, D)
        float moveSpeed = 0.1f; // Adjust movement speed as needed
        if (IsKeyDown(KEY_W)) ray.position.z -= moveSpeed; // Move forward
        if (IsKeyDown(KEY_S)) ray.position.z += moveSpeed; // Move backward
        if (IsKeyDown(KEY_A)) ray.position.x -= moveSpeed; // Move left
        if (IsKeyDown(KEY_D)) ray.position.x += moveSpeed; // Move right

        // Check ray collision with cube
        collision = GetRayCollisionBox(ray,
                            (BoundingBox){
                                (Vector3){ cubePosition.x - cubeSize.x/2, cubePosition.y - cubeSize.y/2, cubePosition.z - cubeSize.z/2 },
                                (Vector3){ cubePosition.x + cubeSize.x/2, cubePosition.y + cubeSize.y/2, cubePosition.z + cubeSize.z/2 }
                            });

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                // Draw cube
                DrawCube(cubePosition, 1.0f, 1.0f, 1.0f, collision.hit ? GREEN : RED); // Change color on hit
                DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);

                DrawGrid(10, 1.0f);
                DrawRay(ray, MAROON);

            EndMode3D();

            // Draw UI
            DrawText("Welcome to the third dimension!", 10, 40, 20, DARKGRAY);
            DrawText("Use W,S,A,D to move ray", 10, 60, 20, DARKGRAY);
            DrawText(collision.hit ? "Collision Detected!" : "No Collision", 10, 80, 20, collision.hit ? GREEN : DARKGRAY);
            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}