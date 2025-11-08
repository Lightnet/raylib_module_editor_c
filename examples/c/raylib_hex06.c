/*********************************************************************
 *  raylib 5.5 – Hex-Bar Progress Bar (position + width/height)
 *  Compile (MinGW example):
 *      gcc hexbar.c -o hexbar -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
 *********************************************************************/

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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

/* ---------- FIXED LEFT-CAP ONLY ---------- */
static void DrawHexBarFill(const Vector2 *v, float progress, Color col)
{
    if (progress <= 0.0f) return;
    if (progress >= 1.0f) {
        const Vector2 fan[7] = { v[0], v[1], v[2], v[3], v[4], v[5], v[0] };
        DrawTriangleFan(fan, 7, col);
        return;
    }

    /* ----- geometry ----- */
    const float leftCapLen  = v[0].x - v[5].x;   // distance from tip to flat side
    const float rectLen     = v[1].x - v[0].x;
    const float rightCapLen = v[2].x - v[1].x;
    const float totalLen    = leftCapLen + rectLen + rightCapLen;
    const float fillLen     = totalLen * progress;

    /* ---------- 1. LEFT CAP (fixed) ---------- */
    if (fillLen <= leftCapLen) {
        /* t = 0 → at the tip (v[5])
         * t = 1 → at the flat side (v[0] / v[4])          */
        const float t = fillLen / leftCapLen;

        /* points that move from the tip toward the flat side */
        const Vector2 top    = Vector2Lerp(v[5], v[0], t);
        const Vector2 bottom = Vector2Lerp(v[5], v[4], t);

        /* draw the growing triangle */
        DrawTriangle(v[5], top, bottom, col);
        return;
    }

    /* left cap is now full – draw the whole left triangle */
    DrawTriangle(v[5], v[0], v[4], col);

    /* ----- the rest of your original code (unchanged) ----- */
    const float afterLeft = fillLen - leftCapLen;

    if (afterLeft <= rectLen) {
        const float w = afterLeft;
        DrawRectangleV((Vector2){v[0].x, v[0].y},
                       (Vector2){w, v[4].y - v[0].y}, col);
        return;
    }
    DrawRectangleV((Vector2){v[0].x, v[0].y},
                   (Vector2){rectLen, v[4].y - v[0].y}, col);

    const float afterRect = afterLeft - rectLen;
    const float t = afterRect / rightCapLen;
    const Vector2 a = Vector2Lerp(v[1], v[2], t);
    const Vector2 b = Vector2Lerp(v[3], v[2], t);
    DrawTriangle(v[2], a, b, col);
}

/* ---------- Main ---------- */
int main(void)
{
    const int scrW = 1000;
    const int scrH = 600;
    InitWindow(scrW, scrH, "Hex-Bar – Fully Fixed Fill");
    SetTargetFPS(60);

    const float cx = scrW * 0.5f;
    const float cy = scrH * 0.5f;
    const float width  = 400.0f;
    const float height = 100.0f;

    float progress = 0.0f;
    const float speed = 0.6f;  // for demo animation

    char txt[16];

    while (!WindowShouldClose())
    {
        // Animate progress
        progress += speed * GetFrameTime();
        if (progress > 1.1f) progress = 0.0f;

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