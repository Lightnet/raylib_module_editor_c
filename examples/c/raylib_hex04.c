#include "raylib.h"
#include <math.h>

/* -------------------------------------------------------------
   HUD hexagon that auto-sizes to the text
   ------------------------------------------------------------- */
static void DrawHalfHexTextBox(const char *text, int fontSize,
                               int hudHeight, Color boxColor,
                               Color textColor, Color bgColor)
{
    const int  screenW = GetScreenWidth();
    const int  screenH = GetScreenHeight();
    const int  hudY    = screenH - hudHeight;          // HUD top line

    // ---- 1. measure text -------------------------------------------------
    int textW = MeasureText(text, fontSize);
    const int padding = 24;                            // left/right space
    int neededW = textW + 2*padding;                   // inner width we need

    // ---- 2. geometry of a full hexagon ----------------------------------
    //   flat-top hex: width = 2 * radius
    //   pointy-top hex: width = √3 * radius
    // We use **flat-top** (rotation = 0) because the top side is flat.
    const bool flatTop = true;
    const float rotation = 0.0f;                       // flat top

    // For the **top half** the vertical span is radius (center → top vertex)
    // → radius = hudHeight / 2
    float radius = hudHeight / 2.0f;

    // ---- 3. compute centre so that the *top* of the half-hex touches hudY
    //   centre.y = hudY + radius   (top vertex at y = centre.y - radius)
    Vector2 centre = { screenW/2.0f, hudY + radius };

    // ---- 4. make the hexagon wide enough for the text --------------------
    //   Full width of a flat-top hex = 2 * radius
    //   We need at least `neededW` → adjust radius if necessary
    float minRadius = neededW / 2.0f;                   // flat-top formula
    if (radius < minRadius) radius = minRadius;

    // ---- 5. optional: clamp maximum width (e.g. 80 % of screen)
    float maxRadius = screenW * 0.40f;                  // 80 % of screen width
    if (radius > maxRadius) radius = maxRadius;

    // ---- 6. draw HUD background (transparent) ---------------------------
    DrawRectangle(0, hudY, screenW, hudHeight, Fade(bgColor, 0.45f));

    // ---- 7. build the 4 vertices of the top half ------------------------
    //   0° (right), 60°, 120°, 180° (left) → flat top
    Vector2 verts[4];
    for (int i = 0; i < 4; ++i)
    {
        float angle = (float)i * (PI / 3.0f);          // 0, 60, 120, 180 deg
        verts[i].x = centre.x + radius * cosf(angle);
        verts[i].y = centre.y + radius * sinf(angle);
    }

    // ---- 8. draw the filled half-hex (triangle fan) --------------------
    //   centre → v0 → v1 → v2 → v3 → v0
    DrawTriangleFan(verts, 4, Fade(boxColor, 0.85f));

    // ---- 9. outline (optional) -----------------------------------------
    DrawLineStrip(verts, 4, WHITE);                 // closes automatically
    // close the shape manually if you want a thicker line
    DrawLineEx(verts[3], verts[0], 2.0f, WHITE);

    // ---- 10. draw centred text -----------------------------------------
    DrawText(text,
             (int)(centre.x - textW/2.0f),
             (int)(centre.y - fontSize/2.0f),
             fontSize, textColor);
}

int main(void)
{
    const int screenWidth  = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib 5.5 – Half-Hex Text Box");
    SetTargetFPS(60);

    const int hudHeight = screenHeight / 10;   // 10 % of screen height
    const char *msg     = "Player Health: 85%";
    const int  fontSize = 26;

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);

            // ---- your game scene ------------------------------------------------
            DrawRectangle(0, 0, screenWidth, screenHeight - hudHeight,
                          Fade(BLUE, 0.7f));

            // ---- draw the half-hex text box ------------------------------------
            DrawHalfHexTextBox(msg, fontSize, hudHeight,
                               GREEN, WHITE, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}