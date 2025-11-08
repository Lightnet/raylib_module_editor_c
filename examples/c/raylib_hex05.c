#include "raylib.h"
#include <math.h>
#include <stdio.h>

/* --------------------------------------------------------------
   Helper: return a vertex of the hexagon given an angle (degrees)
   -------------------------------------------------------------- */
static Vector2 GetHexVertex(Vector2 center, float radius, float angleDeg)
{
    const float rad = DEG2RAD * angleDeg;
    return (Vector2){
        center.x + radius * cosf(rad),
        center.y + radius * sinf(rad)
    };
}

int main(void)
{
    /* ---------------------  Window init  --------------------- */
    const int screenWidth  = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib 5.5 – Hex Progress Bar");
    SetTargetFPS(60);

    /* ---------------------  Hexagon params  --------------------- */
    const Vector2 center = { screenWidth / 2.0f, screenHeight / 2.0f };
    const float   radius = 100.0f;          // distance centre → vertex
    float         progress = 0.75f;         // 0.0 … 1.0
    char          txt[16];

    /* ---------------------------------------------------------- */
    while (!WindowShouldClose())
    {
        /* ---- optional animated progress (uncomment to test) ----
        progress = (sinf((float)GetTime()) + 1.0f) * 0.5f;
        --------------------------------------------------------- */

        snprintf(txt, sizeof(txt), "%.0f%%", progress * 100.0f);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        /* ----- 1. Draw the full hexagon outline (6 lines) ----- */
        for (int i = 0; i < 6; ++i)
        {
            const float a1 = 60.0f * i;          // start vertex
            const float a2 = 60.0f * (i + 1);    // end vertex
            const Vector2 v1 = GetHexVertex(center, radius, a1);
            const Vector2 v2 = GetHexVertex(center, radius, a2);
            DrawLineV(v1, v2, GRAY);
        }

        /* ----- 2. Fill the progress segment (clockwise from top) ----- */
        const int   fullSeg   = (int)(6 * progress);                 // whole triangles
        const float fracAngle = 60.0f * (progress * 6.0f - fullSeg); // partial triangle

        /* full triangles */
        for (int i = 0; i < fullSeg; ++i)
        {
            const float a1 = -30.0f + 60.0f * i;   // -30° → pointy-top orientation
            const float a2 = a1 + 60.0f;
            const Vector2 v1 = GetHexVertex(center, radius, a1);
            const Vector2 v2 = GetHexVertex(center, radius, a2);
            DrawTriangle(center, v1, v2, Fade(BLUE, 0.8f));
        }

        /* partial triangle (if any) */
        if (fracAngle > 0.0f)
        {
            const float a1 = -30.0f + 60.0f * fullSeg;
            const float a2 = a1 + fracAngle;
            const Vector2 v1 = GetHexVertex(center, radius, a1);
            const Vector2 v2 = GetHexVertex(center, radius, a2);
            DrawTriangle(center, v1, v2, Fade(BLUE, 0.8f));
        }

        /* ----- 3. Draw centered percentage text ----- */
        const int   fontSize = 24;
        const Vector2 txtSize = MeasureTextEx(GetFontDefault(), txt, (float)fontSize, 1.0f);
        const Vector2 txtPos  = {
            center.x - txtSize.x * 0.5f,
            center.y - txtSize.y * 0.5f
        };
        DrawText(txt, (int)txtPos.x, (int)txtPos.y, fontSize, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}