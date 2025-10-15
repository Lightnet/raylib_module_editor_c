#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
// #define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "ode/ode.h"

// Physics objects
dWorldID world;
dSpaceID space;
dGeomID ground;
dBodyID cube_body;
dGeomID cube_geom;
dJointGroupID contact_group;

// Debug mode flag
bool debug_mode = false;

// Store contact points for visualization (optional)
#define MAX_CONTACTS 4
Vector3 contact_points[MAX_CONTACTS];
int contact_count = 0;

// Callback for collision detection
static void nearCallback(void *data, dGeomID o1, dGeomID o2) {
    dWorldID world = (dWorldID)data;
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    if (b1 && b2 && dAreConnected(b1, b2)) return;

    dContact contacts[MAX_CONTACTS];
    int n = dCollide(o1, o2, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));

    // Store contact points for debug visualization
    if (debug_mode) {
        contact_count = n;
        for (int i = 0; i < n && i < MAX_CONTACTS; ++i) {
            contact_points[i] = (Vector3){
                (float)contacts[i].geom.pos[0],
                (float)contacts[i].geom.pos[1],
                (float)contacts[i].geom.pos[2]
            };
        }
    }

    for (int i = 0; i < n; ++i) {
        contacts[i].surface.mode = dContactBounce;
        contacts[i].surface.mu = dInfinity;
        contacts[i].surface.bounce = 0.5f;
        contacts[i].surface.bounce_vel = 0.1f;
        contacts[i].surface.soft_cfm = 0.01f;
        dJointID c = dJointCreateContact(world, contact_group, &contacts[i]);
        dJointAttach(c, b1, b2);
    }
}

// Function to reset cube position and rotation randomly
void resetCubePosition(dBodyID body) {
    float x = (float)GetRandomValue(-50, 50) / 10.0f; // -5 to 5
    float z = (float)GetRandomValue(-50, 50) / 10.0f;
    float y = (float)GetRandomValue(50, 150) / 10.0f; // 5 to 15

    // Random rotation: generate random Euler angles (yaw, pitch, roll) in degrees
    float yaw = (float)GetRandomValue(0, 3600) / 10.0f;   // 0 to 360 degrees
    float pitch = (float)GetRandomValue(-900, 900) / 10.0f; // -90 to 90 degrees (to avoid full flips that might cause issues)
    float roll = (float)GetRandomValue(0, 3600) / 10.0f;

    // Convert Euler angles to rotation matrix (ODE uses column-major matrix)
    dMatrix3 rotation;
    dRFromEulerAngles(rotation, roll * DEG2RAD, pitch * DEG2RAD, yaw * DEG2RAD);

    dBodySetPosition(body, x, y, z);
    dBodySetRotation(body, rotation);
    dBodySetLinearVel(body, 0, 0, 0);
    dBodySetAngularVel(body, 0, 0, 0);
}

// Convert ODE rotation matrix to yaw, pitch, roll (in degrees)
void getYawPitchRoll(const dReal *rot, float *yaw, float *pitch, float *roll) {
    float r11 = rot[0], r12 = rot[4], r13 = rot[8];
    float r21 = rot[1], r23 = rot[9];
    float r31 = rot[2], r32 = rot[6], r33 = rot[10];

    *pitch = atan2f(-r31, sqrtf(r11 * r11 + r21 * r21)) * RAD2DEG;
    *yaw = atan2f(r21, r11) * RAD2DEG;
    *roll = atan2f(r32, r33) * RAD2DEG;
}

Matrix ode_to_rl_transform(dBodyID _body) {
    const dReal *pos = dBodyGetPosition(_body);
    const dReal *rot = dBodyGetRotation(_body);

    Matrix rot_matrix = {
        (float)rot[0], (float)rot[1], (float)rot[2],  0.0f,
        (float)rot[4], (float)rot[5], (float)rot[6],  0.0f,
        (float)rot[8], (float)rot[9], (float)rot[10], 0.0f,
        0.0f,          0.0f,          0.0f,          1.0f
    };

    Matrix trans_matrix = MatrixTranslate((float)pos[0], (float)pos[1], (float)pos[2]);
    return MatrixMultiply(rot_matrix, trans_matrix);
}

int main() {
    // Initialize ODE
    dInitODE();
    world = dWorldCreate();
    space = dHashSpaceCreate(0);
    contact_group = dJointGroupCreate(0);
    dWorldSetGravity(world, 0, -9.81f, 0);

    // Create ground plane
    ground = dCreatePlane(space, 0, 1, 0, 0);

    // Define cube size
    const float cube_size = 1.0f;

    // Create cube
    cube_body = dBodyCreate(world);
    dMass mass;
    dMassSetBox(&mass, 1.0, cube_size, cube_size, cube_size);
    dBodySetMass(cube_body, &mass);
    cube_geom = dCreateBox(space, cube_size, cube_size, cube_size);
    dGeomSetBody(cube_geom, cube_body);
    resetCubePosition(cube_body);

    // Initialize Raylib
    InitWindow(800, 600, "Cube Drop Simulation (ODE) - Click Buttons to Reset/Toggle Debug");
    SetTargetFPS(60);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model cube_model = LoadModelFromMesh(GenMeshCube(cube_size, cube_size, cube_size));
    SetRandomSeed((unsigned int)GetTime());

    // Main loop
    while (!WindowShouldClose()) {
        // Update physics
        dSpaceCollide(space, world, &nearCallback);
        dWorldQuickStep(world, 1.0f / 60.0f);
        dJointGroupEmpty(contact_group);

        // Update cube transformation
        const dReal *pos = dBodyGetPosition(cube_body);
        const dReal *rot = dBodyGetRotation(cube_body);
        cube_model.transform = ode_to_rl_transform(cube_body);

        // Get yaw, pitch, roll for display
        float yaw, pitch, roll;
        getYawPitchRoll(rot, &yaw, &pitch, &roll);

        // Begin drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);

        // Draw ground plane
        DrawPlane((Vector3){0, 0, 0}, (Vector2){10, 10}, GRAY);

        // Draw cube
        DrawModel(cube_model, (Vector3){0, 0, 0}, 1.0f, RED);
        // Draw wireframe (debug mode uses lime for visibility)
        Color wire_color = debug_mode ? LIME : BLACK;
        DrawModelWires(cube_model, (Vector3){0, 0, 0}, 1.0f, wire_color);

        // Draw ground plane wireframe in debug mode (optional)
        if (debug_mode) {
            DrawGrid(20, 1.0f); // Larger grid for better visibility
        }

        // Draw contact points in debug mode (optional)
        if (debug_mode) {
            for (int i = 0; i < contact_count; ++i) {
                DrawSphere(contact_points[i], 0.05f, YELLOW); // Solid small spheres for contacts
                DrawSphereWires(contact_points[i], 0.05f, 8, 8, YELLOW);
            }
        }

        EndMode3D();

        // Draw GUI: Reset button
        Rectangle resetButtonBounds = { 10, 10, 200, 30 };
        if (GuiButton(resetButtonBounds, "Reset Cube (Random Rot)")) {
            resetCubePosition(cube_body);
        }

        // Draw GUI: Debug toggle button
        Rectangle debugButtonBounds = { 220, 10, 200, 30 };
        if (GuiButton(debugButtonBounds, debug_mode ? "Debug: ON" : "Debug: OFF")) {
            debug_mode = !debug_mode;
        }

        // Draw HUD
        DrawFPS(10, 50);
        DrawText("Buttons: Reset sets random position + rotation", 10, 80, 20, DARKGRAY);
        DrawText(TextFormat("Position: X: %.2f  Y: %.2f  Z: %.2f", pos[0], pos[1], pos[2]), 10, 110, 20, DARKGRAY);
        DrawText(TextFormat("Rotation: Yaw: %.1f  Pitch: %.1f  Roll: %.1f", yaw, pitch, roll), 10, 140, 20, DARKGRAY);
        if (debug_mode) {
            DrawText(TextFormat("Contacts: %d", contact_count), 10, 170, 20, DARKGRAY);
        }

        EndDrawing();
    }

    // Visio
    UnloadModel(cube_model);
    dJointGroupDestroy(contact_group);
    dSpaceDestroy(space);
    dWorldDestroy(world);
    dCloseODE();
    CloseWindow();

    return 0;
}