// main.c - Raylib 5.5 Noise Texture Pixel Scaler with raygui + stb_perlin
// Compile with: gcc main.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
// stb_perlin.h: https://github.com/nothings/stb/blob/master/stb_perlin.h

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BASE_SIZE 16

// Custom function to generate Perlin noise image using stb_perlin
Image GenImageSTBPerlinNoise(int width, int height, int offsetX, int offsetY, float scale, float lacunarity, float gain, int octaves)
{
    Image image = { 0 };
    image.data = malloc(width * height * 4);  // RGBA, 4 bytes per pixel
    image.width = width;
    image.height = height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    unsigned char* pixels = (unsigned char*)image.data;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Use ridged multifractal noise
            float noise = stb_perlin_ridge_noise3((offsetX + x) * scale, (offsetY + y) * scale, 0.0f, lacunarity, gain, 1.0f, octaves);
            // Remap [-1,1] to [0,255] for grayscale
            unsigned char gray = (unsigned char)((noise + 1.0f) * 127.5f);

            int index = (y * width + x) * 4;
            pixels[index + 0] = gray;     // R
            pixels[index + 1] = gray;     // G
            pixels[index + 2] = gray;     // B
            pixels[index + 3] = 255;      // A (opaque)
        }
    }

    return image;
}

int main(void)
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Noise Texture Pixel Scaler - raygui + stb_perlin");
    SetTargetFPS(60);

    // Seed random for potential variations
    srand(time(NULL));

    // Noise parameters
    float noiseScale = 0.1f;    // Noise frequency
    float lacunarity = 2.0f;    // Frequency multiplier per octave
    float gain = 0.5f;          // Amplitude multiplier per octave
    int octaves = 3;            // Number of noise layers

    // Initialize static variables for tracking parameter changes
    static float prevNoiseScale;
    static float prevLacunarity;
    static float prevGain;
    static int prevOctaves;
    prevNoiseScale = noiseScale;
    prevLacunarity = lacunarity;
    prevGain = gain;
    prevOctaves = octaves;

    // Generate initial 16x16 noise texture
    Image imgNoise = GenImageSTBPerlinNoise(BASE_SIZE, BASE_SIZE, 0, 0, noiseScale, lacunarity, gain, octaves);
    Texture2D texNoise = LoadTextureFromImage(imgNoise);
    UnloadImage(imgNoise);

    // Set texture filter to POINT for pixelated scaling
    SetTextureFilter(texNoise, TEXTURE_FILTER_POINT);

    // GUI state variables
    float scaleFactor = 10.0f;  // Texture scaling factor
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
            int randOffsetX = rand() % 100;
            int randOffsetY = rand() % 100;
            Image imgNew = GenImageSTBPerlinNoise(BASE_SIZE, BASE_SIZE, randOffsetX, randOffsetY, noiseScale, lacunarity, gain, octaves);
            texNoise = LoadTextureFromImage(imgNew);
            UnloadImage(imgNew);
            SetTextureFilter(texNoise, TEXTURE_FILTER_POINT);
            regenerate = false;
        }

        // Check for parameter changes to regenerate texture
        if (noiseScale != prevNoiseScale || lacunarity != prevLacunarity || gain != prevGain || octaves != prevOctaves)
        {
            UnloadTexture(texNoise);
            Image imgNew = GenImageSTBPerlinNoise(BASE_SIZE, BASE_SIZE, rand() % 100, rand() % 100, noiseScale, lacunarity, gain, octaves);
            texNoise = LoadTextureFromImage(imgNew);
            UnloadImage(imgNew);
            SetTextureFilter(texNoise, TEXTURE_FILTER_POINT);
            prevNoiseScale = noiseScale;
            prevLacunarity = lacunarity;
            prevGain = gain;
            prevOctaves = octaves;
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
        DrawTexturePro(texNoise, sourceRec, destRec, origin, 0.0f, WHITE);

        // Draw info text
        DrawText(TextFormat("16x16 Noise Texture scaled to %dx%d pixels", (int)(BASE_SIZE * scaleFactor), (int)(BASE_SIZE * scaleFactor)), 10, 10, 20, DARKGRAY);

        // raygui controls panel
        Rectangle panelRec = { 60.0f, 50.0f, 200.0f, 300.0f };  // Expanded height for more sliders
        DrawRectangleRec(panelRec, Fade(LIGHTGRAY, 0.8f));

        int sliderY = 60;
        int spacing = 50;

        // Texture scale slider
        GuiSlider((Rectangle){ panelRec.x + 10, sliderY, panelRec.width - 20, 20 }, "Tex Scale", TextFormat("%.1f", scaleFactor), &scaleFactor, minScale, maxScale);
        sliderY += spacing;

        // Noise scale slider
        GuiSlider((Rectangle){ panelRec.x + 10, sliderY, panelRec.width - 20, 20 }, "Noise Scale", TextFormat("%.2f", noiseScale), &noiseScale, 0.01f, 0.5f);
        sliderY += spacing;

        // Lacunarity slider
        GuiSlider((Rectangle){ panelRec.x + 10, sliderY, panelRec.width - 20, 20 }, "Lacunarity", TextFormat("%.1f", lacunarity), &lacunarity, 1.0f, 3.0f);
        sliderY += spacing;

        // Gain slider
        GuiSlider((Rectangle){ panelRec.x + 10, sliderY, panelRec.width - 20, 20 }, "Gain", TextFormat("%.1f", gain), &gain, 0.1f, 1.0f);
        sliderY += spacing;

        // Octaves slider
        float octavesFloat = (float)octaves;
        GuiSlider((Rectangle){ panelRec.x + 10, sliderY, panelRec.width - 20, 20 }, "Octaves", TextFormat("%d", octaves), &octavesFloat, 1.0f, 6.0f);
        octaves = (int)octavesFloat;  // Convert back to int
        sliderY += spacing;

        // Regenerate button
        if (GuiButton((Rectangle){ panelRec.x + 10, sliderY, panelRec.width - 20, 30 }, "New Noise"))
        {
            regenerate = true;
        }

        // Info label
        DrawText("Custom 16x16 pixel\nstb_perlin noise texture", panelRec.x + 10, sliderY + 40, 12, DARKGRAY);

        EndDrawing();
    }

    // De-Initialization
    UnloadTexture(texNoise);
    CloseWindow();

    return 0;
}