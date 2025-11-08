// main.c - Sci-Fi Hex Text Message Box with raylib 5.5
#include "raylib.h"
#include <math.h>

#define SCREEN_WIDTH  1200
#define SCREEN_HEIGHT  700

void DrawHexagon(Vector2 center, float radius, Color color);

int main(void)
{
    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sci-Fi Hex Message Box - raylib 5.5");
    SetTargetFPS(60);

    // Message box properties
    Rectangle msgBox = { 200, 150, 800, 400 };
    const char *message = "SYSTEM ALERT:\n\nQuantum core destabilizing...\nInitiating emergency protocol.\n\n> Press [ENTER] to continue";
    int fontSize = 28;
    Font font = GetFontDefault();

    // Hex grid settings
    float hexRadius = 30.0f;
    float hexAlpha = 0.08f;

    // Glow pulse
    float glowPulse = 0.0f;

    while (!WindowShouldClose())
    {
        // Update
        glowPulse += 0.05f;
        if (glowPulse > 6.28f) glowPulse = 0.0f;

        // Begin drawing
        BeginDrawing();
        ClearBackground(BLACK);

        // === 1. Draw faint hexagonal grid background ===
        for (float y = -hexRadius; y < SCREEN_HEIGHT + hexRadius; y += hexRadius * 1.732f)
        {
            for (float x = -hexRadius; x < SCREEN_WIDTH + hexRadius; x += hexRadius * 3.0f)
            {
                float offset = (y / (hexRadius * 1.732f)) * (hexRadius * 1.5f);
                DrawHexagon((Vector2){x + offset, y}, hexRadius, Fade(WHITE, hexAlpha));
                DrawHexagon((Vector2){x + offset + hexRadius * 1.5f, y}, hexRadius, Fade(WHITE, hexAlpha));
            }
        }

        // === 2. Draw multi-layered glowing border (sci-fi effect) ===
        float glow = 8.0f + 4.0f * sinf(glowPulse);  // Pulsing intensity
        Color glowColor = WHITE;

        // Outer glow layers
        for (int i = 5; i >= 1; i--)
        {
            float thickness = i * 1.5f + glow;
            DrawRectangleLinesEx(
                (Rectangle){msgBox.x - thickness, msgBox.y - thickness,
                            msgBox.width + thickness*2, msgBox.height + thickness*2},
                thickness, Fade(glowColor, 0.1f * i)
            );
        }

        // Sharp white outline
        DrawRectangleLinesEx(msgBox, 3.0f, WHITE);

        // === 3. Draw message box background (semi-transparent black) ===
        DrawRectangleRec(msgBox, Fade(BLACK, 0.85f));

        // === 4. Draw sci-fi text with scanlines ===
        Vector2 textPos = { msgBox.x + 40, msgBox.y + 40 };
        int lineHeight = fontSize + 8;

        // Split message into lines
        const char *lines[] = {
            "SYSTEM ALERT:",
            "",
            "Quantum core destabilizing...",
            "Initiating emergency protocol.",
            "",
            "> Press [ENTER] to continue"
        };
        int numLines = 6;

        for (int i = 0; i < numLines; i++)
        {
            if (lines[i][0] == '\0') {
                textPos.y += lineHeight * 0.5f;
                continue;
            }

            // Main text
            DrawTextEx(font, lines[i], textPos, fontSize, 2, WHITE);

            // CRT scanline effect (subtle horizontal lines over text)
            if (i < 5) {
                int scanY = (int)(textPos.y + fontSize * 0.7f);
                for (int s = 0; s < 3; s++) {
                    DrawRectangle(msgBox.x, scanY + s, (int)msgBox.width, 1,
                                  Fade(CLITERAL(Color){0, 255, 200, 20}, 0.3f));
                }
            }

            textPos.y += lineHeight;
        }

        // === 5. Corner hex accents (decorative) ===
        Vector2 corners[4] = {
            {msgBox.x, msgBox.y},
            {msgBox.x + msgBox.width, msgBox.y},
            {msgBox.x + msgBox.width, msgBox.y + msgBox.height},
            {msgBox.x, msgBox.y + msgBox.height}
        };

        for (int i = 0; i < 4; i++)
        {
            DrawHexagon(corners[i], 20, Fade(WHITE, 0.3f + 0.2f * sinf(glowPulse + i)));
            DrawHexagon(corners[i], 12, WHITE);
        }

        // === 6. HUD-style top bar label ===
        DrawRectangle(msgBox.x, msgBox.y - 40, msgBox.width, 3, WHITE);
        DrawTextEx(font, "INCOMING TRANSMISSION", (Vector2){msgBox.x + 20, msgBox.y - 55}, 20, 1, Fade(WHITE, 0.8f));

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

// Helper: Draw regular hexagon
void DrawHexagon(Vector2 center, float radius, Color color)
{
    const int sides = 6;
    Vector2 points[sides];

    for (int i = 0; i < sides; i++)
    {
        float angle = i * 2 * PI / sides - PI / 6;  // Rotate -30° to flat-top
        points[i] = (Vector2){
            center.x + radius * cosf(angle),
            center.y + radius * sinf(angle)
        };
    }

    // Draw filled hex
    for (int i = 0; i < sides; i++)
    {
        DrawTriangleFan(points, sides, color);
        break; // DrawTriangleFan fills it
    }

    // Outline
    for (int i = 0; i < sides; i++)
    {
        DrawLineEx(points[i], points[(i+1)%sides], 1.5f, color);
    }
}