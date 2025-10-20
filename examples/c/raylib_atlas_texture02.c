#include "raylib.h"
#include <stddef.h>  // For NULL

// Function to crop a 16x16 tile from a 64x64 atlas based on index (0 to 15 for a 4x4 grid)
Texture2D LoadTileTexture(Image* atlasImage, int tileIndex)
{
    // Ensure index is valid (0 to 15 for a 4x4 grid)
    if (tileIndex < 0 || tileIndex > 15) {
        TraceLog(LOG_WARNING, "Invalid tile index: %d, using index 0", tileIndex);
        tileIndex = 0;
    }

    // Calculate tile position (16x16 tiles in a 64x64 atlas)
    int tileX = (tileIndex % 4) * 16;  // Column (0, 16, 32, 48)
    int tileY = (tileIndex / 4) * 16;  // Row (0, 16, 32, 48)

    // Crop the 16x16 tile
    Image tileImage = ImageCopy(*atlasImage);  // Copy to avoid modifying original
    ImageCrop(&tileImage, (Rectangle){(float)tileX, (float)tileY, 16, 16});
    ImageFormat(&tileImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);  // Ensure RGBA
    Texture2D tileTexture = LoadTextureFromImage(tileImage);
    UnloadImage(tileImage);  // Free cropped image
    return tileTexture;
}

int main(void)
{
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Raylib: 16x16 Texture Atlas on Cube");
    SetTargetFPS(60);

    // Load texture atlas (ensure resources/simple_color64x64.png exists)
    Image atlasImage = LoadImage("resources/simple_color64x64.png");
    if (atlasImage.data == NULL) {
        TraceLog(LOG_ERROR, "Failed to load texture: resources/simple_color64x64.png");
        CloseWindow();
        return 1;
    }

    // Load initial tile texture (index 0 = top-left tile)
    int currentTileIndex = 0;
    Texture2D atlasTexture = LoadTileTexture(&atlasImage, currentTileIndex);

    // Create cube mesh and model
    Mesh cubeMesh = GenMeshCube(2.0f, 2.0f, 2.0f);  // Cube with size 2x2x2
    Model cubeModel = LoadModelFromMesh(cubeMesh);
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
        // Update tile index with number keys (0-9 for indices 0-9, for simplicity)
        for (int i = 0; i <= 9; i++) {
            if (IsKeyPressed(KEY_ZERO + i)) {
                currentTileIndex = i;
                // Unload previous texture and load new tile texture
                UnloadTexture(atlasTexture);
                atlasTexture = LoadTileTexture(&atlasImage, currentTileIndex);
                SetMaterialTexture(&cubeModel.materials[0], MATERIAL_MAP_DIFFUSE, atlasTexture);
            }
        }

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
        DrawText(TextFormat("Tile Index: %d (Press 0-9 to change)", currentTileIndex), 10, 30, 20, DARKGRAY);
        DrawText("Use mouse to orbit camera around the textured cube!", 10, 50, 20, DARKGRAY);
        EndDrawing();
    }

    // De-Initialization
    UnloadImage(atlasImage);  // Free original atlas image
    UnloadModel(cubeModel);   // Unloads mesh and material
    UnloadTexture(atlasTexture);
    CloseWindow();

    return 0;
}