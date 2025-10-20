// main.c - Raylib 5.5 Noise Texture Pixel Scaler with raygui
// Compile with: gcc main.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
// https://www.raylib.com/examples/textures/loader.html?name=textures_image_generation

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BASE_SIZE 16

int main(void)
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Noise Texture Pixel Scaler - raygui");
    SetTargetFPS(60);

    // Seed random for noise generation
    srand(time(NULL));

    // Generate 16x16 noise texture
    // Image imgNoise = GenImageWhiteNoise(BASE_SIZE, BASE_SIZE, 1.0f);  // White noise, scale 1.0
    Image imgNoise = GenImagePerlinNoise(BASE_SIZE, BASE_SIZE, 50, 50, 1.0f);
    Texture2D texNoise = LoadTextureFromImage(imgNoise);
    UnloadImage(imgNoise);

    // Set texture filter to POINT for pixelated scaling
    SetTextureFilter(texNoise, TEXTURE_FILTER_POINT);

    // GUI state variables
    float scaleFactor = 10.0f;  // Initial scale to 160x160 pixels
    float minScale = 1.0f;
    float maxScale = 50.0f;
    bool regenerate = false;

    // Main loop
    while (!WindowShouldClose())
    {
        // Update
        if (regenerate)
        {
            UnloadTexture(texNoise);
            // Image imgNew = GenImageWhiteNoise(BASE_SIZE, BASE_SIZE, (float)(rand() % 100) / 100.0f);  // Random scale for new noise
            Image imgNew = GenImagePerlinNoise(BASE_SIZE, BASE_SIZE, 50, 50, 1.0f);
            texNoise = LoadTextureFromImage(imgNew);
            UnloadImage(imgNew);
            SetTextureFilter(texNoise, TEXTURE_FILTER_POINT);
            regenerate = false;
        }

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw scaled texture in center
        Rectangle sourceRec = { 0.0f, 0.0f, (float)texNoise.width, (float)texNoise.height };
        Rectangle destRec = {
            (SCREEN_WIDTH - (float)(BASE_SIZE * scaleFactor)) / 2.0f,
            (SCREEN_HEIGHT - (float)(BASE_SIZE * scaleFactor)) / 2.0f,
            (float)(BASE_SIZE * scaleFactor),
            (float)(BASE_SIZE * scaleFactor)
        };
        Vector2 origin = { 0.0f, 0.0f };
        // DrawTexturePro(texNoise, sourceRec, destRec, origin, 0.0f, BLACK);
        DrawTexturePro(texNoise, sourceRec, destRec, origin, 0.0f, WHITE);

        // DrawTexture(texNoise, 0, 0, WHITE);

        // Draw info text
        DrawText(TextFormat("16x16 Noise Texture scaled to %dx%d pixels", (int)(BASE_SIZE * scaleFactor), (int)(BASE_SIZE * scaleFactor)), 10, 10, 20, DARKGRAY);

        // raygui controls panel
        Rectangle panelRec = { 10.0f, 50.0f, 200.0f, 200.0f };
        DrawRectangleRec(panelRec, Fade(LIGHTGRAY, 0.8f));

        int sliderY = 60;
        int buttonY = 120;
        int labelY = 160;

        // Scale slider - Pass pointer to scaleFactor
        GuiSlider((Rectangle){ panelRec.x + 10, sliderY, panelRec.width - 20, 20 }, "Scale", TextFormat("%.0f", scaleFactor), &scaleFactor, minScale, maxScale);

        // Regenerate button
        if (GuiButton((Rectangle){ panelRec.x + 10, buttonY, panelRec.width - 20, 30 }, "New Noise"))
        {
            regenerate = true;
        }

        // Info label
        DrawText("Custom 16x16 pixel\nnoise texture", panelRec.x + 10, labelY, 12, DARKGRAY);

        EndDrawing();
    }

    // De-Initialization
    UnloadTexture(texNoise);
    CloseWindow();

    return 0;
}