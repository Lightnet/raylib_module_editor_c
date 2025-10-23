/*******************************************************************************************
*
*   LayoutTransform3d v1.0.0 - Tool Description
*
*   LICENSE: Propietary License
*
*   Copyright (c) 2022 raylib technologies. All Rights Reserved.
*
*   Unauthorized copying of this file, via any medium is strictly prohibited
*   This project is proprietary and confidential unless the owner allows
*   usage in any other form by expresely written permission.
*
**********************************************************************************************/

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    // Initialization
    //---------------------------------------------------------------------------------------
    int screenWidth = 800;
    int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "layout_transform3d");

    // layout_transform3d: controls initialization
    //----------------------------------------------------------------------------------
    bool WindowBox000Active = true;
    bool ValueBOx001EditMode = false;
    int ValueBOx001Value = 0;
    bool ValueBOx002EditMode = false;
    int ValueBOx002Value = 0;
    bool ValueBOx003EditMode = false;
    int ValueBOx003Value = 0;
    bool ValueBOx004EditMode = false;
    int ValueBOx004Value = 0;
    bool ValueBOx005EditMode = false;
    int ValueBOx005Value = 0;
    bool ValueBOx006EditMode = false;
    int ValueBOx006Value = 0;
    bool ValueBOx007EditMode = false;
    int ValueBOx007Value = 0;
    bool ValueBOx008EditMode = false;
    int ValueBOx008Value = 0;
    bool ValueBOx009EditMode = false;
    int ValueBOx009Value = 0;
    int ToggleGroup010Active = 0;
    //----------------------------------------------------------------------------------

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Implement required update logic
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR))); 

            // raygui: controls drawing
            //----------------------------------------------------------------------------------
            if (WindowBox000Active)
            {
                WindowBox000Active = !GuiWindowBox((Rectangle){ 24, 16, 192, 328 }, "Transform");
                if (GuiValueBox((Rectangle){ 64, 96, 120, 24 }, "p x: ", &ValueBOx001Value, 0, 100, ValueBOx001EditMode)) ValueBOx001EditMode = !ValueBOx001EditMode;
                if (GuiValueBox((Rectangle){ 64, 120, 120, 24 }, "p y: ", &ValueBOx002Value, 0, 100, ValueBOx002EditMode)) ValueBOx002EditMode = !ValueBOx002EditMode;
                if (GuiValueBox((Rectangle){ 64, 144, 120, 24 }, "p z: ", &ValueBOx003Value, 0, 100, ValueBOx003EditMode)) ValueBOx003EditMode = !ValueBOx003EditMode;
                if (GuiValueBox((Rectangle){ 64, 176, 120, 24 }, "r x: ", &ValueBOx004Value, 0, 100, ValueBOx004EditMode)) ValueBOx004EditMode = !ValueBOx004EditMode;
                if (GuiValueBox((Rectangle){ 64, 200, 120, 24 }, "r y: ", &ValueBOx005Value, 0, 100, ValueBOx005EditMode)) ValueBOx005EditMode = !ValueBOx005EditMode;
                if (GuiValueBox((Rectangle){ 64, 224, 120, 24 }, "r z:", &ValueBOx006Value, 0, 100, ValueBOx006EditMode)) ValueBOx006EditMode = !ValueBOx006EditMode;
                if (GuiValueBox((Rectangle){ 64, 256, 120, 24 }, "s x: ", &ValueBOx007Value, 0, 100, ValueBOx007EditMode)) ValueBOx007EditMode = !ValueBOx007EditMode;
                if (GuiValueBox((Rectangle){ 64, 280, 120, 24 }, "s y: ", &ValueBOx008Value, 0, 100, ValueBOx008EditMode)) ValueBOx008EditMode = !ValueBOx008EditMode;
                if (GuiValueBox((Rectangle){ 64, 304, 120, 24 }, "s z: ", &ValueBOx009Value, 0, 100, ValueBOx009EditMode)) ValueBOx009EditMode = !ValueBOx009EditMode;
                GuiToggleGroup((Rectangle){ 48, 48, 40, 24 }, "ONE;TWO;THREE", &ToggleGroup010Active);
            }
            //----------------------------------------------------------------------------------

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Controls Functions Definitions (local)
//------------------------------------------------------------------------------------

