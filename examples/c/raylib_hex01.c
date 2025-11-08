/*********************************************************************
 *  raylib 5.5 + raygui – Hex-Bar Progress Bar (100% fill)
 *  gcc raylib_hex01.c -o hexbar -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
 *********************************************************************/

#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raymath.h"
#include "raygui.h"
#include <stdio.h>
#include <math.h>  // at top with other includes

/* ---------- Vertices ---------- */
static void GetHexBarVertices(Vector2 *out, float cx, float cy,
                              float width, float height)
{
    const float hw = width  * 0.5f;
    const float hh = height * 0.5f;
    out[0] = (Vector2){ cx - hw,          cy - hh };   // top-left
    out[1] = (Vector2){ cx + hw,          cy - hh };   // top-right
    out[2] = (Vector2){ cx + hw + hh,     cy };       // right tip
    out[3] = (Vector2){ cx + hw,          cy + hh };   // bottom-right
    out[4] = (Vector2){ cx - hw,          cy + hh };   // bottom-left
    out[5] = (Vector2){ cx - hw - hh,     cy };       // left tip
}

/* ---------- Outline ---------- */
static void DrawHexBarOutline(const Vector2 *v, Color col)
{
    for (int i = 0; i < 6; ++i)
        DrawLineV(v[i], v[(i+1)%6], col);
}



/* ---------- PERFECT Fill (left-cap → rect → right-cap) ---------- */
static void DrawHexBarFill(const Vector2 *v, float progress, Color col)
{
    if (progress <= 0.0f) return;
    if (progress >= 1.0f) progress = 1.0f;

    const float capLen  = v[0].x - v[5].x;          // length of one cap
    const float rectLen = v[1].x - v[0].x;          // length of the flat middle part
    if (capLen <= 0.0f || rectLen <= 0.0f) return;

    const float totalLen = capLen + rectLen + capLen;
    const float fillLen  = totalLen * progress;

    const float leftCapEnd = capLen;
    const float rectEnd    = capLen + rectLen;

    /* ---------- 1. LEFT CAP ---------- */
    if (fillLen <= leftCapEnd) {
        const float t = fillLen / capLen;
        const Vector2 top    = Vector2Lerp(v[5], v[0], t);
        const Vector2 bottom = Vector2Lerp(v[5], v[4], t);
        DrawTriangle(v[5], bottom, top, col);
        return;
    }
    DrawTriangle(v[5], v[4], v[0], col);

    /* ---------- 2. RECTANGLE ---------- */
    if (fillLen <= rectEnd) {
        const float rectFill = fillLen - capLen;
        DrawRectangleV((Vector2){v[0].x, v[0].y},
                       (Vector2){rectFill, v[4].y - v[0].y}, col);
        return;
    }
    DrawRectangleV((Vector2){v[0].x, v[0].y},
                   (Vector2){rectLen, v[4].y - v[0].y}, col);


    /* ---------- 3. RIGHT CAP – PERFECT FILL, CORRECT WINDING ---------- */
    const float rightCapFill = fillLen - rectEnd;
    if (rightCapFill > 0.0f) {
        const float t = fminf(rightCapFill / capLen, 1.0f);
        const Vector2 top    = Vector2Lerp(v[1], v[2], t);
        const Vector2 bottom = Vector2Lerp(v[3], v[2], t);

        if (t >= 1.0f) {
            // Full cap: CCW order
            DrawTriangle(v[1], v[3], v[2], col);  // v1 -> v3 -> v2 (CCW)
        } else {
            // Partial fill: draw trapezoid v1 -> top -> bottom -> v3

            // Triangle 1: v1 -> v3 -> top  (CCW)
            DrawTriangle(v[1], v[3], top, col);

            // Triangle 2: v3 -> bottom -> top  (CCW)
            DrawTriangle(v[3], bottom, top, col);
        }
    }

}

/* ---------- Main ---------- */
int main(void)
{
    const int scrW = 1000;
    const int scrH = 600;
    InitWindow(scrW, scrH, "Hex-Bar – 100% Perfect Fill");
    SetTargetFPS(60);

    const float cx = scrW * 0.5f;
    const float cy = scrH * 0.3f;
    const float width  = 400.0f;
    const float height = 100.0f;

    float progress = 0.0f;
    char txt[16];

    while (!WindowShouldClose())
    {
        /* ----- SLIDER ----- */
        Rectangle sliderRec = { scrW*0.5f - 200.0f, scrH*0.6f, 400.0f, 20.0f };
        // GuiSliderBar(sliderRec, "Progress:", "0% - 100%", &progress, 0.0f, 1.0f);
        GuiSliderBar(sliderRec, "Progress:", TextFormat("%.0f%%", progress*100), &progress, 0.0f, 1.0f);

        snprintf(txt, sizeof(txt), "%.0f%%", progress*100.0f);

        Vector2 v[6];
        GetHexBarVertices(v, cx, cy, width, height);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawHexBarOutline(v, DARKGRAY);
        DrawHexBarFill(v, progress, Fade(BLUE, 0.8f));

        const int fs = 32;
        Vector2 ts = MeasureTextEx(GetFontDefault(), txt, (float)fs, 1.0f);
        DrawText(txt, (int)(cx - ts.x*0.5f), (int)(cy - ts.y*0.5f), fs, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}