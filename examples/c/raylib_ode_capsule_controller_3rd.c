#include "raylib.h"
#include "raymath.h"
#include <ode/ode.h>
#include <stdio.h>
#include <math.h> // Ensure math.h is included for PI

// Global ODE variables
dWorldID world;
dSpaceID space;
dJointGroupID contactgroup;
dGeomID floorGeom, cube1Geom, cube2Geom;
dBodyID cube1Body, cube2Body;
Model cubeModel;

// capsule variables
dGeomID capsuleGeom;
dBodyID capsuleBody;
Model capsuleModel;
// Model capsuleModel2; //test

// Global variable for the fixed joint
dJointID capsuleFixedJoint;

// Global variables for camera
bool cameraControlEnabled = false; // Toggle for mouse controls
float cameraAngleX = -133.0f; // Horizontal angle (degrees, yaw)
float cameraAngleY = -33.0f; // Vertical angle (degrees, pitch)

// Contact callback for collision handling
void nearCallback(void *data, dGeomID o1, dGeomID o2) {
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    dContact contact;
    contact.surface.mode = dContactBounce;
    // contact.surface.mu = dInfinity; // Friction
    contact.surface.mu = 50.0f; // Friction
    contact.surface.bounce = 0.5f;  // Restitution
    if (dCollide(o1, o2, 1, &contact.geom, sizeof(dContact))) {
        dJointID c = dJointCreateContact(world, contactgroup, &contact);
        dJointAttach(c, b1, b2);
    }
}

float NormalizeAngle(float angle) {
    // Normalize angle to [-π, π]
    angle = fmod(angle, 2.0f * PI);
    if (angle > PI) {
        angle -= 2.0f * PI;
    } else if (angle < -PI) {
        angle += 2.0f * PI;
    }
    return angle;
}

float NormalizeAngleDegrees(float angle) {
    // Normalize angle to [-180, 180]
    angle = fmod(angle, 360.0f);
    if (angle > 180.0f) {
        angle -= 360.0f;
    } else if (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}

// Convert ODE quaternion to Raylib Matrix
Matrix ODEQuaternionToMatrix(dQuaternion q) {
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

// generate capsule, z axis to match ODE capsule
Mesh GenMeshCapsule(float radius, float height, int slices, int rings) {
    Mesh mesh = { 0 };
    float halfHeight = height / 2.0f;
    int ringsPerHemisphere = rings / 2; // Rings per hemisphere, excluding poles
    // Vertex count: 1 pole + ringsPerHemisphere * (slices + 1) for top hemisphere,
    // + (ringsPerHemisphere + 1) * (slices + 1) for bottom hemisphere (include last ring),
    // + 2 cylinder rings
    int vertexCount = 1 + ringsPerHemisphere * (slices + 1) + (ringsPerHemisphere + 1) * (slices + 1) + 2 * (slices + 1) + 1;
    // Triangle count: (slices + (ringsPerHemisphere - 1) * slices * 2) for top hemisphere
    // + (slices + ringsPerHemisphere * slices * 2) for bottom hemisphere + slices * 2 for cylinder
    int triangleCount = (slices + (ringsPerHemisphere - 1) * slices * 2) + (slices + ringsPerHemisphere * slices * 2) + slices * 2;
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = triangleCount;
    mesh.vertices = (float *)RL_MALLOC(vertexCount * 3 * sizeof(float));
    mesh.normals = (float *)RL_MALLOC(vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(vertexCount * 2 * sizeof(float));
    mesh.indices = (unsigned short *)RL_MALLOC(triangleCount * 3 * sizeof(unsigned short));

    int v = 0, i = 0;

    // Top pole (single vertex at z = halfHeight + radius)
    printf("Top Pole Vertex:\n");
    mesh.vertices[v * 3 + 0] = 0.0f;
    mesh.vertices[v * 3 + 1] = 0.0f;
    mesh.vertices[v * 3 + 2] = halfHeight + radius;
    mesh.normals[v * 3 + 0] = 0.0f;
    mesh.normals[v * 3 + 1] = 0.0f;
    mesh.normals[v * 3 + 2] = 1.0f;
    mesh.texcoords[v * 2 + 0] = 0.5f;
    mesh.texcoords[v * 2 + 1] = 0.0f;
    printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, mesh.vertices[v * 3 + 0], mesh.vertices[v * 3 + 1], mesh.vertices[v * 3 + 2]);
    v++;

    // Top hemisphere (z from halfHeight to halfHeight + radius, excluding pole)
    printf("\nTop Hemisphere Vertices:\n");
    for (int j = 1; j <= ringsPerHemisphere; j++) {
        float phi = (float)j * PI / 2.0f / ringsPerHemisphere; // 0 to PI/2
        for (int k = 0; k <= slices; k++) {
            float theta = (float)k * 2.0f * PI / slices;
            float sinPhi = sinf(phi);
            float x = radius * sinPhi * cosf(theta);
            float y = radius * sinPhi * sinf(theta);
            float z = radius * cosf(phi) + halfHeight;
            mesh.vertices[v * 3 + 0] = x;
            mesh.vertices[v * 3 + 1] = y;
            mesh.vertices[v * 3 + 2] = z;
            mesh.normals[v * 3 + 0] = x / radius;
            mesh.normals[v * 3 + 1] = y / radius;
            mesh.normals[v * 3 + 2] = (z - halfHeight) / radius;
            mesh.texcoords[v * 2 + 0] = (float)k / slices;
            mesh.texcoords[v * 2 + 1] = (float)j / (ringsPerHemisphere * 2);
            printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, x, y, z);
            v++;
        }
    }

    // Cylinder top ring (at z = halfHeight)
    printf("\nCylinder Top Ring Vertices:\n");
    for (int k = 0; k <= slices; k++) {
        float theta = (float)k * 2.0f * PI / slices;
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        float z = halfHeight;
        mesh.vertices[v * 3 + 0] = x;
        mesh.vertices[v * 3 + 1] = y;
        mesh.vertices[v * 3 + 2] = z;
        mesh.normals[v * 3 + 0] = x / radius;
        mesh.normals[v * 3 + 1] = y / radius;
        mesh.normals[v * 3 + 2] = 0.0f;
        mesh.texcoords[v * 2 + 0] = (float)k / slices;
        mesh.texcoords[v * 2 + 1] = 0.5f;
        printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, x, y, z);
        v++;
    }

    // Cylinder bottom ring (at z = -halfHeight)
    printf("\nCylinder Bottom Ring Vertices:\n");
    for (int k = 0; k <= slices; k++) {
        float theta = (float)k * 2.0f * PI / slices;
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        float z = -halfHeight;
        mesh.vertices[v * 3 + 0] = x;
        mesh.vertices[v * 3 + 1] = y;
        mesh.vertices[v * 3 + 2] = z;
        mesh.normals[v * 3 + 0] = x / radius;
        mesh.normals[v * 3 + 1] = y / radius;
        mesh.normals[v * 3 + 2] = 0.0f;
        mesh.texcoords[v * 2 + 0] = (float)k / slices;
        mesh.texcoords[v * 2 + 1] = 0.5f;
        printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, x, y, z);
        v++;
    }

    // Bottom hemisphere (z from -halfHeight to just before -halfHeight - radius)
    printf("\nBottom Hemisphere Vertices:\n");
    for (int j = 0; j <= ringsPerHemisphere; j++) { // Include last ring
        float phi = PI / 2.0f + (float)j * PI / 2.0f / ringsPerHemisphere; // PI/2 to PI
        for (int k = 0; k <= slices; k++) {
            float theta = (float)k * 2.0f * PI / slices;
            float sinPhi = sinf(phi);
            float x = radius * sinPhi * cosf(theta);
            float y = radius * sinPhi * sinf(theta);
            float z = radius * cosf(phi) - halfHeight;
            mesh.vertices[v * 3 + 0] = x;
            mesh.vertices[v * 3 + 1] = y;
            mesh.vertices[v * 3 + 2] = z;
            mesh.normals[v * 3 + 0] = x / radius;
            mesh.normals[v * 3 + 1] = y / radius;
            mesh.normals[v * 3 + 2] = (z + halfHeight) / radius;
            mesh.texcoords[v * 2 + 0] = (float)k / slices;
            mesh.texcoords[v * 2 + 1] = (float)(j + ringsPerHemisphere) / (ringsPerHemisphere * 2);
            printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, x, y, z);
            v++;
        }
    }

    // Bottom pole (single vertex at z = -halfHeight - radius)
    printf("\nBottom Pole Vertex:\n");
    mesh.vertices[v * 3 + 0] = 0.0f;
    mesh.vertices[v * 3 + 1] = 0.0f;
    mesh.vertices[v * 3 + 2] = -halfHeight - radius;
    mesh.normals[v * 3 + 0] = 0.0f;
    mesh.normals[v * 3 + 1] = 0.0f;
    mesh.normals[v * 3 + 2] = -1.0f;
    mesh.texcoords[v * 2 + 0] = 0.5f;
    mesh.texcoords[v * 2 + 1] = 1.0f;
    printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, mesh.vertices[v * 3 + 0], mesh.vertices[v * 3 + 1], mesh.vertices[v * 3 + 2]);
    v++;

    // Indices for top hemisphere (including pole)
    int topHemisphereStart = 1; // After top pole
    printf("\nTop Hemisphere Indices:\n");
    // Pole triangles (connect pole to first ring)
    for (int k = 0; k < slices; k++) {
        int v0 = 0; // Top pole
        int v1 = topHemisphereStart + k;
        int v2 = topHemisphereStart + (k + 1) % (slices + 1);
        mesh.indices[i++] = v0;
        mesh.indices[i++] = v1;
        mesh.indices[i++] = v2;
        printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v0, v1, v2);
    }
    // Remaining top hemisphere quads
    for (int j = 0; j < ringsPerHemisphere - 1; j++) {
        for (int k = 0; k < slices; k++) {
            int v0 = topHemisphereStart + j * (slices + 1) + k;
            int v1 = v0 + 1;
            int v2 = topHemisphereStart + (j + 1) * (slices + 1) + k;
            int v3 = v2 + 1;
            mesh.indices[i++] = v0;
            mesh.indices[i++] = v2;
            mesh.indices[i++] = v1;
            mesh.indices[i++] = v1;
            mesh.indices[i++] = v2;
            mesh.indices[i++] = v3;
            printf("Triangle %d: (%d, %d, %d)\n", i/3-2, v0, v2, v1);
            printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v1, v2, v3);
        }
    }

    // Indices for cylinder (connect top ring to bottom ring)
    int topRingOffset = topHemisphereStart + ringsPerHemisphere * (slices + 1);
    int bottomRingOffset = topRingOffset + (slices + 1);
    printf("\nCylinder Indices:\n");
    for (int k = 0; k < slices; k++) {
        int v0 = topRingOffset + k;
        int v1 = v0 + 1;
        int v2 = bottomRingOffset + k;
        int v3 = v2 + 1;
        mesh.indices[i++] = v0;
        mesh.indices[i++] = v2;
        mesh.indices[i++] = v1;
        mesh.indices[i++] = v1;
        mesh.indices[i++] = v2;
        mesh.indices[i++] = v3;
        printf("Triangle %d: (%d, %d, %d)\n", i/3-2, v0, v2, v1);
        printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v1, v2, v3);
    }

    // Indices for bottom hemisphere (including pole)
    int bottomHemisphereOffset = bottomRingOffset + (slices + 1);
    int bottomPole = bottomHemisphereOffset + ringsPerHemisphere * (slices + 1);
    printf("\nBottom Hemisphere Indices:\n");
    // Bottom hemisphere quads
    for (int j = 0; j < ringsPerHemisphere; j++) {
        for (int k = 0; k < slices; k++) {
            int v0 = bottomHemisphereOffset + j * (slices + 1) + k;
            int v1 = v0 + 1;
            int v2 = bottomHemisphereOffset + (j + 1) * (slices + 1) + k;
            int v3 = v2 + 1;
            mesh.indices[i++] = v0;
            mesh.indices[i++] = v2;
            mesh.indices[i++] = v1;
            mesh.indices[i++] = v1;
            mesh.indices[i++] = v2;
            mesh.indices[i++] = v3;
            printf("Triangle %d: (%d, %d, %d)\n", i/3-2, v0, v2, v1);
            printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v1, v2, v3);
        }
    }
    // Pole triangles (connect last ring to bottom pole)
    for (int k = 0; k < slices; k++) {
        int v0 = bottomPole;
        int v1 = bottomHemisphereOffset + ringsPerHemisphere * (slices + 1) + k;
        int v2 = bottomHemisphereOffset + ringsPerHemisphere * (slices + 1) + (k + 1) % (slices + 1);
        mesh.indices[i++] = v0;
        mesh.indices[i++] = v2; // Reversed for correct winding
        mesh.indices[i++] = v1;
        printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v0, v2, v1);
    }

    // Upload mesh data to GPU
    UploadMesh(&mesh, false);
    return mesh;
}

// main
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

    // Create capsule model
    Mesh capsuleMesh = GenMeshCapsule(0.5f, 1.0f, 8, 8); // Radius 0.5, cylinder height 1.0
    capsuleModel = LoadModelFromMesh(capsuleMesh);
    // capsuleModel2 = capsuleModel; // Assuming capsuleModel2 is for testing; copy the model

    // Initialize ODE
    dInitODE();
    world = dWorldCreate();
    space = dHashSpaceCreate(0);
    contactgroup = dJointGroupCreate(0);
    dWorldSetGravity(world, 0, -9.81f, 0); // Gravity along Y-axis

    // Create static floor (cube)
    floorGeom = dCreateBox(space, 10.0f, 1.0f, 10.0f);
    dGeomSetPosition(floorGeom, 0.0f, -0.5f, 0.0f);

    // Store initial capsule position for reset
    Vector3 initialCube1Pos = (Vector3){ -1.0f, 1.5f, 0.0f };
    Vector3 initialCube2Pos = (Vector3){ -2.0f, 3.5f, 0.0f }; // Adjusted to stack cubes
    Vector3 initialCapsulePos = (Vector3){ 0.0f, 2.5f, 0.0f };

    // Create first dynamic cube
    cube1Body = dBodyCreate(world);
    cube1Geom = dCreateBox(space, 1.0f, 1.0f, 1.0f);
    dMass mass1;
    dMassSetBox(&mass1, 1.0f, 1.0f, 1.0f, 1.0f); // Density = 1.0
    dBodySetMass(cube1Body, &mass1);
    dGeomSetBody(cube1Geom, cube1Body);
    dBodySetPosition(cube1Body, initialCube1Pos.x, initialCube1Pos.y, initialCube1Pos.z);

    // Create second dynamic cube (stacked on top)
    cube2Body = dBodyCreate(world);
    cube2Geom = dCreateBox(space, 1.0f, 1.0f, 1.0f);
    dMass mass2;
    dMassSetBox(&mass2, 1.0f, 1.0f, 1.0f, 1.0f); // Density = 1.0
    dBodySetMass(cube2Body, &mass2);
    dGeomSetBody(cube2Geom, cube2Body);
    dBodySetPosition(cube2Body, initialCube2Pos.x, initialCube2Pos.y, initialCube2Pos.z);

    // Create capsule in ODE
    capsuleBody = dBodyCreate(world);
    capsuleGeom = dCreateCapsule(space, 0.5f, 1.0f); // Radius 0.5, cylinder length 1.0
    dMass massCapsule;
    dMassSetCapsule(&massCapsule, 1.0f, 2, 0.5f, 1.0f); // Density = 1.0, direction along y-axis (2)
    dBodySetMass(capsuleBody, &massCapsule);
    dGeomSetBody(capsuleGeom, capsuleBody);
    dQuaternion yAxisQuat;
    dQFromAxisAndAngle(yAxisQuat, 1.0f, 0.0f, 0.0f, PI / 2.0f); // Rotate z to y
    dBodySetQuaternion(capsuleBody, yAxisQuat);
    dBodySetPosition(capsuleBody, initialCapsulePos.x, initialCapsulePos.y, initialCapsulePos.z);
    dBodySetMaxAngularSpeed(capsuleBody, 0.0); // Prevent rotation for capsule

    // Add toggle for capsule movement
    bool capsuleMovementEnabled = false;

    while (!WindowShouldClose()) {
        // Step ODE simulation
        dSpaceCollide(space, 0, &nearCallback);
        dWorldQuickStep(world, 0.016f); // ~60 FPS
        dJointGroupEmpty(contactgroup);

        // Toggle capsule movement with '1' key
        if (IsKeyPressed(KEY_ONE)) {
            capsuleMovementEnabled = !capsuleMovementEnabled;
        }

        // Reset both cubes and capsule on 'R' key press
        if (IsKeyPressed(KEY_R)) {
            dBodySetPosition(cube1Body, initialCube1Pos.x, initialCube1Pos.y, initialCube1Pos.z);
            dBodySetLinearVel(cube1Body, 0, 0, 0);
            dBodySetAngularVel(cube1Body, 0, 0, 0);
            dQuaternion identity1 = { 1.0f, 0.0f, 0.0f, 0.0f };
            dBodySetQuaternion(cube1Body, identity1);

            dBodySetPosition(cube2Body, initialCube2Pos.x, initialCube2Pos.y, initialCube2Pos.z);
            dBodySetLinearVel(cube2Body, 0, 0, 0);
            dBodySetAngularVel(cube2Body, 0, 0, 0);
            dQuaternion identity2 = { 1.0f, 0.0f, 0.0f, 0.0f };
            dBodySetQuaternion(cube2Body, identity2);

            dBodySetPosition(capsuleBody, initialCapsulePos.x, initialCapsulePos.y, initialCapsulePos.z);
            dBodySetLinearVel(capsuleBody, 0, 0, 0);
            dBodySetAngularVel(capsuleBody, 0, 0, 0);
            dQFromAxisAndAngle(yAxisQuat, 1.0f, 0.0f, 0.0f, PI / 2.0f);
            dBodySetQuaternion(capsuleBody, yAxisQuat);
        }

        // Toggle camera controls with Tab key
        if (IsKeyPressed(KEY_TAB)) {
            cameraControlEnabled = !cameraControlEnabled;
            if (cameraControlEnabled) {
                DisableCursor();
            } else {
                EnableCursor();
            }
        }

        // === UPDATE MOUSE LOOK (TAB toggles) ===
        if (cameraControlEnabled) {
            Vector2 mouseDelta = GetMouseDelta();
            // FIX: Use consistent mouse direction (no inversion)
            cameraAngleX -= mouseDelta.x * 0.3f * -1;  // Left = positive yaw (standard)
            cameraAngleY -= mouseDelta.y * 0.3f;  // Up = positive pitch
            cameraAngleX = NormalizeAngleDegrees(cameraAngleX);
            cameraAngleY = Clamp(cameraAngleY, -89.0f, 89.0f);
        }

        // === CALCULATE CAMERA DIRECTION (from angles) ===
        float yawRad = cameraAngleX * DEG2RAD;
        float pitchRad = cameraAngleY * DEG2RAD;

        Vector3 forward = {
            cosf(pitchRad) * cosf(yawRad),
            sinf(pitchRad),
            cosf(pitchRad) * sinf(yawRad)
        };
        // No need to normalize — cos² + sin² = 1

        Vector3 right = { -sinf(yawRad), 0.0f, cosf(yawRad) };

        // Project to XZ for movement
        Vector3 forwardXZ = { forward.x, 0.0f, forward.z };
        float len = sqrtf(forwardXZ.x*forwardXZ.x + forwardXZ.z*forwardXZ.z);
        if (len > 0.001f) {
            forwardXZ.x /= len;
            forwardXZ.z /= len;
        }

        Vector3 rightXZ = { right.x, 0.0f, right.z };
        len = sqrtf(rightXZ.x*rightXZ.x + rightXZ.z*rightXZ.z);
        if (len > 0.001f) {
            rightXZ.x /= len;
            rightXZ.z /= len;
        }

        // === CAPSULE MOVEMENT (WASD in CAMERA FORWARD DIRECTION) ===
        if (capsuleMovementEnabled) {
            float forceMagnitude = 10.0f;
            Vector3 force = { 0.0f, 0.0f, 0.0f };

            // CORRECT: W = forward, S = back, A = left, D = right
            if (IsKeyDown(KEY_W)) {
                force.x -= rightXZ.x * forceMagnitude;
                force.z -= rightXZ.z * forceMagnitude;
            }
            if (IsKeyDown(KEY_S)) {
                force.x += rightXZ.x * forceMagnitude;
                force.z += rightXZ.z * forceMagnitude;
            }

            if (IsKeyDown(KEY_A)) {
                force.x -= forwardXZ.x * forceMagnitude;
                force.z -= forwardXZ.z * forceMagnitude;
            }

            if (IsKeyDown(KEY_D)) {
                force.x += forwardXZ.x * forceMagnitude;
                force.z += forwardXZ.z * forceMagnitude;
            }

            // === JUMP ===
            if (IsKeyPressed(KEY_SPACE)) {
                const dReal *pos = dBodyGetPosition(capsuleBody);
                if (pos[1] < 1.1f) {
                    dBodyAddForce(capsuleBody, 0.0f, 500.0f, 0.0f);
                }
            }

            dBodyAddForce(capsuleBody, force.x, force.y, force.z);
        }

        // === CAMERA FOLLOW (Third-Person, Behind Capsule) ===
        if (capsuleMovementEnabled) {
            const dReal *pos = dBodyGetPosition(capsuleBody);

            // Offset: behind and above
            Vector3 offset = { 0.0f, 3.0f, 5.0f };

            // Rotate offset by camera yaw (so camera follows behind view direction)
            float cosYaw = cosf(yawRad);
            float sinYaw = sinf(yawRad);
            Vector3 rotated = {
                offset.x * cosYaw - offset.z * sinYaw,
                offset.y,
                offset.x * sinYaw + offset.z * cosYaw
            };

            Vector3 targetPos = {
                (float)pos[0] + rotated.x,
                (float)pos[1] + rotated.y,
                (float)pos[2] + rotated.z
            };

            // Smooth follow
            float t = 8.0f * GetFrameTime();
            camera.position.x = Lerp(camera.position.x, targetPos.x, t);
            camera.position.y = Lerp(camera.position.y, targetPos.y, t);
            camera.position.z = Lerp(camera.position.z, targetPos.z, t);

            // Look at capsule
            camera.target = (Vector3){
                (float)pos[0],
                (float)pos[1] + 0.5f,
                (float)pos[2]
            };
        }
        // === FREE CAMERA (when capsule movement OFF) ===
        else if (cameraControlEnabled) {
            float moveSpeed = 5.0f * GetFrameTime();

            if (IsKeyDown(KEY_W)) {
                camera.position.x += forward.x * moveSpeed;
                camera.position.y += forward.y * moveSpeed;
                camera.position.z += forward.z * moveSpeed;
            }
            if (IsKeyDown(KEY_S)) {
                camera.position.x -= forward.x * moveSpeed;
                camera.position.y -= forward.y * moveSpeed;
                camera.position.z -= forward.z * moveSpeed;
            }
            if (IsKeyDown(KEY_A)) {
                camera.position.x -= right.x * moveSpeed;
                camera.position.z -= right.z * moveSpeed;
            }
            if (IsKeyDown(KEY_D)) {
                camera.position.x += right.x * moveSpeed;
                camera.position.z += right.z * moveSpeed;
            }
            if (IsKeyDown(KEY_SPACE)) camera.position.y += moveSpeed;
            if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.y -= moveSpeed;

            camera.target = Vector3Add(camera.position, forward);
        }
        // === IDLE (no movement) ===
        else {
            camera.target = Vector3Add(camera.position, forward);
        }



        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        DrawCube((Vector3){ 0.0f, -0.5f, 0.0f }, 10.0f, 1.0f, 10.0f, GRAY);
        DrawCubeWires((Vector3){ 0.0f, -0.5f, 0.0f }, 10.0f, 1.0f, 10.0f, BLACK);

        const dReal *cube1Pos = dBodyGetPosition(cube1Body);
        const dReal *cube1QuatPtr = dBodyGetQuaternion(cube1Body);
        dQuaternion cube1Quat = { cube1QuatPtr[0], cube1QuatPtr[1], cube1QuatPtr[2], cube1QuatPtr[3] };
        Matrix cube1RotMatrix = ODEQuaternionToMatrix(cube1Quat);
        cube1RotMatrix.m12 = (float)cube1Pos[0];
        cube1RotMatrix.m13 = (float)cube1Pos[1];
        cube1RotMatrix.m14 = (float)cube1Pos[2];
        cubeModel.transform = cube1RotMatrix;
        DrawModel(cubeModel, (Vector3){0, 0, 0}, 1.0f, RED);
        DrawModelWires(cubeModel, (Vector3){0, 0, 0}, 1.0f, BLACK);

        const dReal *cube2Pos = dBodyGetPosition(cube2Body);
        const dReal *cube2QuatPtr = dBodyGetQuaternion(cube2Body);
        dQuaternion cube2Quat = { cube2QuatPtr[0], cube2QuatPtr[1], cube2QuatPtr[2], cube2QuatPtr[3] };
        Matrix cube2RotMatrix = ODEQuaternionToMatrix(cube2Quat);
        cube2RotMatrix.m12 = (float)cube2Pos[0];
        cube2RotMatrix.m13 = (float)cube2Pos[1];
        cube2RotMatrix.m14 = (float)cube2Pos[2];
        cubeModel.transform = cube2RotMatrix;
        DrawModel(cubeModel, (Vector3){0, 0, 0}, 1.0f, BLUE);
        DrawModelWires(cubeModel, (Vector3){0, 0, 0}, 1.0f, BLACK);

        const dReal *capsulePos = dBodyGetPosition(capsuleBody);
        const dReal *capsuleQuatPtr = dBodyGetQuaternion(capsuleBody);
        dQuaternion capsuleQuat = { capsuleQuatPtr[0], capsuleQuatPtr[1], capsuleQuatPtr[2], capsuleQuatPtr[3] };
        Matrix capsuleRotMatrix = ODEQuaternionToMatrix(capsuleQuat);
        capsuleRotMatrix.m12 = (float)capsulePos[0];
        capsuleRotMatrix.m13 = (float)capsulePos[1];
        capsuleRotMatrix.m14 = (float)capsulePos[2];
        capsuleModel.transform = capsuleRotMatrix;
        DrawModel(capsuleModel, (Vector3){0, 0, 0}, 1.0f, YELLOW);
        DrawModelWires(capsuleModel, (Vector3){0, 0, 0}, 1.0f, BLACK);

        // DrawModel(capsuleModel2, (Vector3){0, 0, 0}, 1.0f, BLUE);
        // DrawModelWires(capsuleModel2, (Vector3){0, 0, 0}, 1.0f, BLACK);

        EndMode3D();

        // Draw instructions
        DrawText("Press 'R' to reset cubes", 10, 10, 20, BLACK);
        DrawText("Press 'TAB' to toggle camera controls", 10, 30, 20, BLACK);
        DrawText("Press '1' to toggle capsule movement", 10, 50, 20, BLACK);
        DrawText(capsuleMovementEnabled ? "Capsule Movement: ON" : "Capsule Movement: OFF", 10, 70, 20, BLACK);
        DrawText(cameraControlEnabled ? "Camera: Mouse active" : "Camera: Mouse inactive", 10, 90, 20, BLACK);
        DrawText(TextFormat("Angle X: %0.02f", cameraAngleX), 10, 110, 20, BLACK);
        DrawText(TextFormat("Angle Y: %0.02f", cameraAngleY), 10, 130, 20, BLACK);
        DrawText("Press 'SPACE' to jump (when movement enabled)", 10, 150, 20, BLACK);

        EndDrawing();
    }

    // Clean up ODE
    dGeomDestroy(floorGeom);
    dGeomDestroy(cube1Geom);
    dGeomDestroy(cube2Geom);
    dGeomDestroy(capsuleGeom);
    dBodyDestroy(cube1Body);
    dBodyDestroy(cube2Body);
    dBodyDestroy(capsuleBody);
    dJointGroupDestroy(contactgroup);
    dSpaceDestroy(space);
    dWorldDestroy(world);
    dCloseODE();

    // Clean up Raylib
    UnloadModel(cubeModel);
    UnloadModel(capsuleModel);
    // UnloadModel(capsuleModel2);
    CloseWindow();

    return 0;
}


