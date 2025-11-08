#include "raylib.h"
#include <math.h>

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib 5.5 - 2D Wave Graph with Grid Style");
    SetTargetFPS(60);

    // Graph parameters
    const int numPoints = 200;
    const float graphLeft = 50.0f;
    const float graphTop = 50.0f;
    const float graphWidth = screenWidth - 100.0f;
    const float graphHeight = screenHeight - 100.0f;
    float time = 0.0f;

    // Grid parameters
    const int gridHSteps = 10;  // Horizontal grid lines
    const int gridVSteps = 20;  // Vertical grid lines

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        time += GetFrameTime();  // Animate the wave

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw grid background (grid style)
        // Horizontal lines
        for (int i = 0; i <= gridHSteps; i++)
        {
            float y = graphTop + (graphHeight * i / gridHSteps);
            DrawLine(graphLeft, y, graphLeft + graphWidth, y, GRAY);
        }
        // Vertical lines
        for (int j = 0; j <= gridVSteps; j++)
        {
            float x = graphLeft + (graphWidth * j / gridVSteps);
            DrawLine(x, graphTop, x, graphTop + graphHeight, GRAY);
        }

        // Draw the 2D wave (sine wave as graph line)
        DrawLine(graphLeft, graphTop + graphHeight / 2, graphLeft + graphWidth, graphTop + graphHeight / 2, DARKGRAY);  // X-axis
        DrawLine(graphLeft, graphTop, graphLeft, graphTop + graphHeight, DARKGRAY);  // Y-axis

        // Calculate and draw wave points
        for (int i = 0; i < numPoints; i++)
        {
            float t = (float)i / (numPoints - 1);  // Normalized time [0,1]
            float x = graphLeft + t * graphWidth;
            float waveY = sinf(t * 4 * PI + time * 2) * (graphHeight / 3) + graphTop + graphHeight / 2;  // Sine wave, scaled and offset
            float nextX = graphLeft + ((float)(i + 1) / numPoints) * graphWidth;
            float nextWaveY = sinf(((float)(i + 1) / numPoints) * 4 * PI + time * 2) * (graphHeight / 3) + graphTop + graphHeight / 2;
            DrawLineEx((Vector2){x, waveY}, (Vector2){nextX, nextWaveY}, 2.0f, BLUE);
        }

        // Draw labels (optional)
        DrawText("2D Wave Graph with Grid", 10, 10, 20, DARKGRAY);
        DrawText("Animated Sine Wave", screenWidth - 200, 10, 20, DARKGRAY);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}