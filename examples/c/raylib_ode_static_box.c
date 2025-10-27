#include "raylib.h"
#include <ode/ode.h>
#include <stdio.h>

// Global ODE variables
dWorldID world;
dSpaceID space;
dJointGroupID contactgroup;
dGeomID floorGeom, cube1Geom, cube2Geom;
dBodyID cube1Body, cube2Body;
Model cubeModel;

// Contact callback for collision handling
void nearCallback(void *data, dGeomID o1, dGeomID o2) {
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    dContact contact;
    contact.surface.mode = dContactBounce;
    contact.surface.mu = dInfinity; // Friction
    contact.surface.bounce = 0.5f;  // Restitution
    if (dCollide(o1, o2, 1, &contact.geom, sizeof(dContact))) {
        dJointID c = dJointCreateContact(world, contactgroup, &contact);
        dJointAttach(c, b1, b2);
    }
}

// Convert ODE quaternion to Raylib Matrix
Matrix QuaternionToMatrix(dQuaternion q) {
    float w = (float)q[0], x = (float)q[1], y = (float)q[2], z = (float)q[3];
    float x2 = x * x, y2 = y * y, z2 = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    // Create 4x4 rotation matrix (column-major for Raylib)
    Matrix mat = {
        1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
        2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx), 0.0f,
        2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    return mat;
}

int main(void) {
    // Initialize window
    InitWindow(800, 600, "3D Physics Cubes with Quaternion Rotation - Raylib with ODE");
    SetTargetFPS(60);

    // Initialize 3D camera
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Create cube model
    Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    cubeModel = LoadModelFromMesh(cubeMesh);

    // Initialize ODE
    dInitODE();
    world = dWorldCreate();
    space = dHashSpaceCreate(0);
    contactgroup = dJointGroupCreate(0);
    dWorldSetGravity(world, 0, -9.81f, 0); // Gravity along Y-axis

    // Create static floor (cube)
    floorGeom = dCreateBox(space, 10.0f, 1.0f, 10.0f);
    dGeomSetPosition(floorGeom, 0.0f, -0.5f, 0.0f);

    // Create first dynamic cube
    cube1Body = dBodyCreate(world);
    cube1Geom = dCreateBox(space, 1.0f, 1.0f, 1.0f);
    dMass mass1;
    dMassSetBox(&mass1, 1.0f, 1.0f, 1.0f, 1.0f); // Density = 1.0
    dBodySetMass(cube1Body, &mass1);
    dGeomSetBody(cube1Geom, cube1Body);
    dBodySetPosition(cube1Body, 0.0f, 1.5f, 0.0f); // Lower cube initial position

    // Create second dynamic cube (stacked on top)
    cube2Body = dBodyCreate(world);
    cube2Geom = dCreateBox(space, 1.0f, 1.0f, 1.0f);
    dMass mass2;
    dMassSetBox(&mass2, 1.0f, 1.0f, 1.0f, 1.0f); // Density = 1.0
    dBodySetMass(cube2Body, &mass2);
    dGeomSetBody(cube2Geom, cube2Body);
    dBodySetPosition(cube2Body, 0.0f, 3.5f, 0.0f); // Upper cube initial position

    // Store initial cube positions for reset
    Vector3 initialCube1Pos = (Vector3){ 0.0f, 1.5f, 0.0f };
    Vector3 initialCube2Pos = (Vector3){ 5.0f, 3.5f, 0.0f };

    while (!WindowShouldClose()) {
        // Step ODE simulation
        dSpaceCollide(space, 0, &nearCallback);
        dWorldQuickStep(world, 0.016f); // ~60 FPS
        dJointGroupEmpty(contactgroup);

        // Reset both cubes on 'R' key press
        if (IsKeyPressed(KEY_R)) {
            // Reset cube1
            dBodySetPosition(cube1Body, initialCube1Pos.x, initialCube1Pos.y, initialCube1Pos.z);
            dBodySetLinearVel(cube1Body, 0, 0, 0);
            dBodySetAngularVel(cube1Body, 0, 0, 0);
            dQuaternion identity1 = { 1.0f, 0.0f, 0.0f, 0.0f }; // Identity quaternion
            dBodySetQuaternion(cube1Body, identity1);

            // Reset cube2
            dBodySetPosition(cube2Body, initialCube2Pos.x, initialCube2Pos.y, initialCube2Pos.z);
            dBodySetLinearVel(cube2Body, 0, 0, 0);
            dBodySetAngularVel(cube2Body, 0, 0, 0);
            dQuaternion identity2 = { 1.0f, 0.0f, 0.0f, 0.0f };
            dBodySetQuaternion(cube2Body, identity2);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // Draw floor
        DrawCube((Vector3){ 0.0f, -0.5f, 0.0f }, 10.0f, 1.0f, 10.0f, GREEN);
        DrawCubeWires((Vector3){ 0.0f, -0.5f, 0.0f }, 10.0f, 1.0f, 10.0f, BLACK);

        // Draw first cube with quaternion rotation
        const dReal *cube1Pos = dBodyGetPosition(cube1Body);
        const dReal *cube1QuatPtr = dBodyGetQuaternion(cube1Body);
        dQuaternion cube1Quat;
        cube1Quat[0] = cube1QuatPtr[0];
        cube1Quat[1] = cube1QuatPtr[1];
        cube1Quat[2] = cube1QuatPtr[2];
        cube1Quat[3] = cube1QuatPtr[3];
        Matrix cube1RotMatrix = QuaternionToMatrix(cube1Quat);
        // Apply translation
        cube1RotMatrix.m12 = (float)cube1Pos[0];
        cube1RotMatrix.m13 = (float)cube1Pos[1];
        cube1RotMatrix.m14 = (float)cube1Pos[2];
        cubeModel.transform = cube1RotMatrix;
        DrawModel(cubeModel, (Vector3){0, 0, 0}, 1.0f, RED);
        DrawModelWires(cubeModel, (Vector3){0, 0, 0}, 1.0f, BLACK);

        // Draw second cube with quaternion rotation
        const dReal *cube2Pos = dBodyGetPosition(cube2Body);
        const dReal *cube2QuatPtr = dBodyGetQuaternion(cube2Body);
        dQuaternion cube2Quat;
        cube2Quat[0] = cube2QuatPtr[0];
        cube2Quat[1] = cube2QuatPtr[1];
        cube2Quat[2] = cube2QuatPtr[2];
        cube2Quat[3] = cube2QuatPtr[3];
        Matrix cube2RotMatrix = QuaternionToMatrix(cube2Quat);
        // Apply translation
        cube2RotMatrix.m12 = (float)cube2Pos[0];
        cube2RotMatrix.m13 = (float)cube2Pos[1];
        cube2RotMatrix.m14 = (float)cube2Pos[2];
        cubeModel.transform = cube2RotMatrix;
        DrawModel(cubeModel, (Vector3){0, 0, 0}, 1.0f, BLUE);
        DrawModelWires(cubeModel, (Vector3){0, 0, 0}, 1.0f, BLACK);

        EndMode3D();

        // Draw instructions
        DrawText("Press 'R' to reset cubes", 10, 10, 20, BLACK);

        EndDrawing();
    }

    // Clean up ODE
    dGeomDestroy(floorGeom);
    dGeomDestroy(cube1Geom);
    dGeomDestroy(cube2Geom);
    dBodyDestroy(cube1Body);
    dBodyDestroy(cube2Body);
    dJointGroupDestroy(contactgroup);
    dSpaceDestroy(space);
    dWorldDestroy(world);
    dCloseODE();

    // Clean up Raylib
    UnloadModel(cubeModel);
    CloseWindow();

    return 0;
}