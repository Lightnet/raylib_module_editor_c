#include "raylib.h"
#include <stddef.h>  // For NULL
#include <string.h>

// Function to create a cube mesh with custom UVs for different textures per face
Mesh GenMeshCubeCustomUV(float width, float height, float length, int tileIndices[6])
{
    Mesh mesh = { 0 };
    mesh.vertexCount = 24;  // 4 vertices per face × 6 faces
    mesh.triangleCount = 12;  // 2 triangles per face × 6 faces
    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

    float w = width * 0.5f;
    float h = height * 0.5f;
    float l = length * 0.5f;

    // Atlas is 64x64, tiles are 16x16, so UVs are in [0,1] normalized coords
    float tileSize = 16.0f / 64.0f;  // 16/64 = 0.25 (normalized tile size)

    // Vertex data for each face (x, y, z)
    float vertices[] = {
        // Front (+Z)
        -w, -h, l,  -w, h, l,   w, h, l,    w, -h, l,
        // Back (-Z)
        w, -h, -l,  w, h, -l,   -w, h, -l,  -w, -h, -l,
        // Left (-X)
        -w, -h, -l, -w, h, -l,  -w, h, l,   -w, -h, l,
        // Right (+X)
        w, -h, l,   w, h, l,    w, h, -l,   w, -h, -l,
        // Top (+Y)
        -w, h, l,   -w, h, -l,  w, h, -l,   w, h, l,
        // Bottom (-Y)
        -w, -h, -l, -w, -h, l,  w, -h, l,   w, -h, -l
    };

    // UVs: Map each face to a specific 16x16 tile based on tileIndices
    float texcoords[48];
    for (int face = 0; face < 6; face++) {
        int tileIndex = tileIndices[face];
        if (tileIndex < 0 || tileIndex > 15) tileIndex = 0;  // Fallback to tile 0
        float u0 = (tileIndex % 4) * tileSize;  // Left edge of tile
        float v0 = (tileIndex / 4) * tileSize;  // Top edge of tile
        float u1 = u0 + tileSize;               // Right edge
        float v1 = v0 + tileSize;               // Bottom edge
        int i = face * 8;  // 4 vertices × 2 coords (u,v)
        texcoords[i + 0] = u0; texcoords[i + 1] = v1;  // Bottom-left
        texcoords[i + 2] = u0; texcoords[i + 3] = v0;  // Top-left
        texcoords[i + 4] = u1; texcoords[i + 5] = v0;  // Top-right
        texcoords[i + 6] = u1; texcoords[i + 7] = v1;  // Bottom-right
    }

    // Indices: Two triangles per face, reversed for correct winding (clockwise)
    unsigned short indices[] = {
        0, 3, 1,  1, 3, 2,      // Front
        4, 7, 5,  5, 7, 6,      // Back
        8, 11, 9, 9, 11, 10,    // Left
        12, 15, 13, 13, 15, 14, // Right
        16, 19, 17, 17, 19, 18, // Top
        20, 23, 21, 21, 23, 22  // Bottom
    };

    memcpy(mesh.vertices, vertices, mesh.vertexCount * 3 * sizeof(float));
    memcpy(mesh.texcoords, texcoords, mesh.vertexCount * 2 * sizeof(float));
    memcpy(mesh.indices, indices, mesh.triangleCount * 3 * sizeof(unsigned short));

    UploadMesh(&mesh, false);  // Upload to GPU
    return mesh;
}

// Update UVs for an existing mesh without rebuilding
void UpdateMeshUVs(Mesh* mesh, int tileIndices[6])
{
    float tileSize = 16.0f / 64.0f;  // 16/64 = 0.25
    float* texcoords = (float*)MemAlloc(mesh->vertexCount * 2 * sizeof(float));
    
    for (int face = 0; face < 6; face++) {
        int tileIndex = tileIndices[face];
        if (tileIndex < 0 || tileIndex > 15) tileIndex = 0;
        float u0 = (tileIndex % 4) * tileSize;
        float v0 = (tileIndex / 4) * tileSize;
        float u1 = u0 + tileSize;
        float v1 = v0 + tileSize;
        int i = face * 8;
        texcoords[i + 0] = u0; texcoords[i + 1] = v1;
        texcoords[i + 2] = u0; texcoords[i + 3] = v0;
        texcoords[i + 4] = u1; texcoords[i + 5] = v0;
        texcoords[i + 6] = u1; texcoords[i + 7] = v1;
    }

    memcpy(mesh->texcoords, texcoords, mesh->vertexCount * 2 * sizeof(float));
    MemFree(texcoords);
    UpdateMeshBuffer(*mesh, 1, mesh->texcoords, mesh->vertexCount * 2 * sizeof(float), 0);  // Update UVs
}

int main(void)
{
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Raylib: Per-Face Textured Cube");
    SetTargetFPS(60);

    // Enable backface culling
    // rlEnableBackfaceCulling();

    // Load texture atlas (ensure resources/simple_color64x64.png exists)
    Image atlasImage = LoadImage("resources/simple_color64x64.png");
    if (atlasImage.data == NULL) {
        TraceLog(LOG_ERROR, "Failed to load texture: resources/simple_color64x64.png");
        CloseWindow();
        return 1;
    }
    ImageFormat(&atlasImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);  // Ensure RGBA
    Texture2D atlasTexture = LoadTextureFromImage(atlasImage);
    UnloadImage(atlasImage);  // Free image

    // Tile indices for each face (front, back, left, right, top, bottom)
    int tileIndices[6] = { 0, 1, 2, 3, 4, 5 };  // Example: different tiles per face
    int selectedFace = 0;  // Face to modify (0-5)

    // Create cube mesh and model
    Mesh cubeMesh = GenMeshCubeCustomUV(2.0f, 2.0f, 2.0f, tileIndices);
    Model cubeModel = LoadModelFromMesh(cubeMesh);
    SetMaterialTexture(&cubeModel.materials[0], MATERIAL_MAP_DIFFUSE, atlasTexture);

    // Camera setup
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 5.0f, 4.0f, 5.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update: Select face with Q/W, change tile with 0-9
        if (IsKeyPressed(KEY_Q)) selectedFace = (selectedFace - 1 + 6) % 6;
        if (IsKeyPressed(KEY_W)) selectedFace = (selectedFace + 1) % 6;
        for (int i = 0; i <= 9; i++) {
            if (IsKeyPressed(KEY_ZERO + i)) {
                tileIndices[selectedFace] = i;
                UpdateMeshUVs(&cubeMesh, tileIndices);  // Update UVs only
            }
        }

        // Update camera
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
            DrawModel(cubeModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
            DrawCubeWires((Vector3){ 0.0f, 0.0f, 0.0f }, 2.0f, 2.0f, 2.0f, MAROON);
        EndMode3D();

        DrawFPS(10, 10);
        DrawText(TextFormat("Selected Face: %d (Q/W to change)", selectedFace), 10, 30, 20, DARKGRAY);
        DrawText(TextFormat("Tile Indices: F:%d B:%d L:%d R:%d T:%d B:%d",
                            tileIndices[0], tileIndices[1], tileIndices[2],
                            tileIndices[3], tileIndices[4], tileIndices[5]), 10, 50, 20, DARKGRAY);
        DrawText("Press 0-9 to change tile, Q/W to select face", 10, 70, 20, DARKGRAY);
        DrawText("Use mouse to orbit camera", 10, 90, 20, DARKGRAY);
        EndDrawing();
    }

    // De-Initialization
    UnloadModel(cubeModel);
    UnloadTexture(atlasTexture);
    CloseWindow();

    return 0;
}