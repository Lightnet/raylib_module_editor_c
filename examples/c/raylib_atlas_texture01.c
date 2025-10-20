#include "raylib.h"
#include <stddef.h>  // For NULL

int main(void)
{
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Raylib: 16x16 Texture Atlas on Cube");
    SetTargetFPS(60);

    // Load and resize texture atlas (ensure resources/simple_color64x64.png exists)
    Image atlasImage = LoadImage("resources/simple_color64x64.png");
    if (atlasImage.data == NULL) {
        TraceLog(LOG_ERROR, "Failed to load texture: resources/simple_color64x64.png");
        CloseWindow();
        return 1;
    }
    // ImageResize(&atlasImage, 16, 16);  // Resize to 16x16

    ImageCrop(&atlasImage, (Rectangle){16,0,16,16});  // crop image pos, 16x16

    ImageFormat(&atlasImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);  // Ensure RGBA format
    Texture2D atlasTexture = LoadTextureFromImage(atlasImage);
    UnloadImage(atlasImage);  // Free image after loading texture

    // Create cube mesh and model
    Mesh cubeMesh = GenMeshCube(2.0f, 2.0f, 2.0f);  // Cube with size 2x2x2
    Model cubeModel = LoadModelFromMesh(cubeMesh);
    // Apply texture to the cube's material
    SetMaterialTexture(&cubeModel.materials[0], MATERIAL_MAP_DIFFUSE, atlasTexture);

    // Cube position
    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };

    // Camera setup for 3D orbit
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 5.0f, 4.0f, 5.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update camera (orbit around cube)
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
            DrawModel(cubeModel, cubePosition, 1.0f, WHITE);  // Draw textured cube
            DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);  // Wireframe for visibility
        EndMode3D();

        DrawFPS(10, 10);
        DrawText("Use mouse to orbit camera around the textured cube!", 10, 30, 20, DARKGRAY);
        EndDrawing();
    }

    // De-Initialization
    UnloadModel(cubeModel);  // Unloads mesh and material
    UnloadTexture(atlasTexture);
    CloseWindow();

    return 0;
}