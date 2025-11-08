#include "raylib.h"


/* -------------------------------------------------------------
   HUD hexagon that auto-sizes to the text
   ------------------------------------------------------------- */
static void DrawHexHUD(const char *text, int fontSize, int hudHeight,
                       Color hexColor, Color textColor, Color bgColor)
{
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    const int hudY    = screenH - hudHeight;

    // ---- 1. measure the text ------------------------------------------------
    int textW = MeasureText(text, fontSize);
    const int padding = 20;                     // space left/right of the text
    int neededW = textW + 2*padding;            // total width we need inside the hex

    // ---- 2. compute the hexagon radius --------------------------------------
    // For a **flat-top** hexagon the distance from centre to a flat side is:
    //   side = radius
    //   width = 2 * radius
    // For a **pointy-top** hexagon the width is:
    //   width = sqrt(3) * radius
    //
    // We want the *inner* width (flat-to-flat) to be at least `neededW`.
    // Add a tiny safety margin so the text never touches the edge.
    const float safety = 4.0f;
    float radius;

    // Choose orientation -------------------------------------------------------
    const bool pointyTop = true;       // false → flat-top, true → pointy-top
    const float rotation = pointyTop ? 30.0f : 0.0f;

    if (pointyTop) {
        // pointy-top → width = √3 * radius
        radius = (neededW + safety) / 1.73205f;   // 1.73205 ≈ √3
    } else {
        // flat-top → width = 2 * radius
        radius = (neededW + safety) / 2.0f;
    }

    // ---- 3. centre of the hexagon (HUD centre) -------------------------------
    Vector2 centre = { screenW/2.0f, hudY + hudHeight/2.0f };

    // ---- 4. draw HUD background (optional) ----------------------------------
    DrawRectangle(0, hudY, screenW, hudHeight, Fade(bgColor, 0.5f));

    // ---- 5. draw the filled hexagon -----------------------------------------
    DrawPoly(centre, 6, radius, rotation, Fade(hexColor, 0.8f));

    // ---- 6. optional outline -------------------------------------------------
    DrawPolyLinesEx(centre, 6, radius, rotation, 2.0f, WHITE);

    // ---- 7. draw the text (perfectly centred) -------------------------------
    DrawText(text,
             (int)(centre.x - textW/2.0f),
             (int)(centre.y - fontSize/2.0f),
             fontSize, textColor);
}




int main(void)
{
    const int screenWidth  = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib 5.5 – Auto-fit Hex HUD");
    SetTargetFPS(60);

    const int hudHeight = screenHeight/10;   // 10 % of screen height
    const char *msg     = "HUD Status";
    const int  fontSize = 24;

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);

            // your game scene …
            DrawRectangle(0, 0, screenWidth, screenHeight-hudHeight, Fade(BLUE, 0.8f));

            // ---- draw the smart hexagon HUD ---------------------------------
            DrawHexHUD(msg, fontSize, hudHeight,
                       GREEN, WHITE, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}