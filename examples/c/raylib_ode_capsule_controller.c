#include "raylib.h"
#include "raymath.h"
#include <ode/ode.h>
#include <stdio.h>
#include <math.h>

// Global ODE variables
dWorldID world;
dSpaceID space;
dJointGroupID contactgroup;
dGeomID floorGeom, cube1Geom, cube2Geom, capsuleGeom;
dBodyID cube1Body, cube2Body, capsuleBody;
Model cubeModel, capsuleModel;

// Camera control
bool cameraControlEnabled = false;
float cameraAngleX = -133.0f;  // Yaw
float cameraAngleY = -33.0f;   // Pitch

// Movement toggle
bool capsuleMovementEnabled = false;

// Contact callback
void nearCallback(void *data, dGeomID o1, dGeomID o2) {
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    dContact contact = {0};
    contact.surface.mode = dContactBounce;
    contact.surface.bounce = 0.5f;

    // Reduce friction only for capsule vs others
    if (o1 == capsuleGeom || o2 == capsuleGeom) {
        contact.surface.mu = 50.0f;
    } else {
        contact.surface.mu = dInfinity;
    }

    if (dCollide(o1, o2, 1, &contact.geom, sizeof(dContact))) {
        dJointID c = dJointCreateContact(world, contactgroup, &contact);
        dJointAttach(c, b1, b2);
    }
}

// Normalize angle to [-180, 180]
float NormalizeAngleDegrees(float angle) {
    angle = fmodf(angle, 360.0f);
    if (angle > 180.0f) angle -= 360.0f;
    if (angle < -180.0f) angle += 360.0f;
    return angle;
}

// ODE quaternion → Raylib Matrix
Matrix ODEQuaternionToMatrix(dQuaternion q) {
    float w = (float)q[0], x = (float)q[1], y = (float)q[2], z = (float)q[3];
    float x2 = x*x, y2 = y*y, z2 = z*z;
    float xy = x*y, xz = x*z, yz = y*z;
    float wx = w*x, wy = w*y, wz = w*z;

    return (Matrix){
        1.0f - 2.0f*(y2 + z2), 2.0f*(xy - wz),       2.0f*(xz + wy),       0.0f,
        2.0f*(xy + wz),       1.0f - 2.0f*(x2 + z2), 2.0f*(yz - wx),       0.0f,
        2.0f*(xz - wy),       2.0f*(yz + wx),       1.0f - 2.0f*(x2 + y2), 0.0f,
        0.0f,                 0.0f,                 0.0f,                 1.0f
    };
}

// Generate capsule mesh (Z-up, matches ODE capsule)
Mesh GenMeshCapsule(float radius, float height, int slices, int rings) {
    Mesh mesh = {0};
    float halfHeight = height * 0.5f;
    int ringsPerHemisphere = rings / 2;

    int vertexCount = 2 + (rings + 2) * (slices + 1);
    int triangleCount = slices * (2 + rings * 2);

    mesh.vertexCount = vertexCount;
    mesh.triangleCount = triangleCount;
    mesh.vertices = RL_MALLOC(vertexCount * 3 * sizeof(float));
    mesh.normals  = RL_MALLOC(vertexCount * 3 * sizeof(float));
    mesh.texcoords = RL_MALLOC(vertexCount * 2 * sizeof(float));
    mesh.indices = RL_MALLOC(triangleCount * 3 * sizeof(unsigned short));

    int v = 0, i = 0;

    // Top pole
    mesh.vertices[v*3+0] = 0; mesh.vertices[v*3+1] = 0; mesh.vertices[v*3+2] = halfHeight + radius;
    mesh.normals[v*3+0] = 0; mesh.normals[v*3+1] = 0; mesh.normals[v*3+2] = 1;
    mesh.texcoords[v*2+0] = 0.5f; mesh.texcoords[v*2+1] = 0.0f; v++;

    // Top hemisphere
    for (int j = 1; j <= ringsPerHemisphere; j++) {
        float phi = j * PI / 2.0f / ringsPerHemisphere;
        for (int k = 0; k <= slices; k++) {
            float theta = k * 2*PI / slices;
            float sp = sinf(phi), cp = cosf(phi);
            float x = radius * sp * cosf(theta);
            float y = radius * sp * sinf(theta);
            float z = radius * cp + halfHeight;
            mesh.vertices[v*3+0] = x; mesh.vertices[v*3+1] = y; mesh.vertices[v*3+2] = z;
            mesh.normals[v*3+0] = x/radius; mesh.normals[v*3+1] = y/radius; mesh.normals[v*3+2] = (z-halfHeight)/radius;
            mesh.texcoords[v*2+0] = (float)k/slices; mesh.texcoords[v*2+1] = (float)j/(ringsPerHemisphere*2);
            v++;
        }
    }

    // Cylinder top ring
    for (int k = 0; k <= slices; k++) {
        float theta = k * 2*PI / slices;
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        mesh.vertices[v*3+0] = x; mesh.vertices[v*3+1] = y; mesh.vertices[v*3+2] = halfHeight;
        mesh.normals[v*3+0] = x/radius; mesh.normals[v*3+1] = y/radius; mesh.normals[v*3+2] = 0;
        mesh.texcoords[v*2+0] = (float)k/slices; mesh.texcoords[v*2+1] = 0.5f;
        v++;
    }

    // Cylinder bottom ring
    for (int k = 0; k <= slices; k++) {
        float theta = k * 2*PI / slices;
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        mesh.vertices[v*3+0] = x; mesh.vertices[v*3+1] = y; mesh.vertices[v*3+2] = -halfHeight;
        mesh.normals[v*3+0] = x/radius; mesh.normals[v*3+1] = y/radius; mesh.normals[v*3+2] = 0;
        mesh.texcoords[v*2+0] = (float)k/slices; mesh.texcoords[v*2+1] = 0.5f;
        v++;
    }

    // Bottom hemisphere
    for (int j = 0; j <= ringsPerHemisphere; j++) {
        float phi = PI/2.0f + j * PI/2.0f / ringsPerHemisphere;
        for (int k = 0; k <= slices; k++) {
            float theta = k * 2*PI / slices;
            float sp = sinf(phi), cp = cosf(phi);
            float x = radius * sp * cosf(theta);
            float y = radius * sp * sinf(theta);
            float z = radius * cp - halfHeight;
            mesh.vertices[v*3+0] = x; mesh.vertices[v*3+1] = y; mesh.vertices[v*3+2] = z;
            mesh.normals[v*3+0] = x/radius; mesh.normals[v*3+1] = y/radius; mesh.normals[v*3+2] = (z+halfHeight)/radius;
            mesh.texcoords[v*2+0] = (float)k/slices; mesh.texcoords[v*2+1] = 0.5f + (float)j/(ringsPerHemisphere*2);
            v++;
        }
    }

    // Bottom pole
    mesh.vertices[v*3+0] = 0; mesh.vertices[v*3+1] = 0; mesh.vertices[v*3+2] = -halfHeight - radius;
    mesh.normals[v*3+0] = 0; mesh.normals[v*3+1] = 0; mesh.normals[v*3+2] = -1;
    mesh.texcoords[v*2+0] = 0.5f; mesh.texcoords[v*2+1] = 1.0f; v++;

    // Indices (simplified, correct winding)
    int topPole = 0;
    int topStart = 1;
    int cylTop = topStart + ringsPerHemisphere * (slices + 1);
    int cylBottom = cylTop + (slices + 1);
    int bottomStart = cylBottom + (slices + 1);
    int bottomPole = bottomStart + (ringsPerHemisphere + 1) * (slices + 1);

    // Top hemisphere
    for (int k = 0; k < slices; k++) {
        mesh.indices[i++] = topPole;
        mesh.indices[i++] = topStart + k;
        mesh.indices[i++] = topStart + (k+1);
    }
    for (int j = 0; j < ringsPerHemisphere - 1; j++) {
        for (int k = 0; k < slices; k++) {
            int a = topStart + j*(slices+1) + k;
            int b = a + 1;
            int c = a + (slices+1);
            int d = c + 1;
            mesh.indices[i++] = a; mesh.indices[i++] = c; mesh.indices[i++] = b;
            mesh.indices[i++] = b; mesh.indices[i++] = c; mesh.indices[i++] = d;
        }
    }

    // Cylinder
    for (int k = 0; k < slices; k++) {
        int a = cylTop + k;
        int b = a + 1;
        int c = cylBottom + k;
        int d = c + 1;
        mesh.indices[i++] = a; mesh.indices[i++] = c; mesh.indices[i++] = b;
        mesh.indices[i++] = b; mesh.indices[i++] = c; mesh.indices[i++] = d;
    }

    // Bottom hemisphere
    for (int j = 0; j < ringsPerHemisphere; j++) {
        for (int k = 0; k < slices; k++) {
            int a = bottomStart + j*(slices+1) + k;
            int b = a + 1;
            int c = a + (slices+1);
            int d = c + 1;
            mesh.indices[i++] = a; mesh.indices[i++] = c; mesh.indices[i++] = b;
            mesh.indices[i++] = b; mesh.indices[i++] = c; mesh.indices[i++] = d;
        }
    }
    for (int k = 0; k < slices; k++) {
        mesh.indices[i++] = bottomPole;
        mesh.indices[i++] = bottomStart + ringsPerHemisphere*(slices+1) + (k+1);
        mesh.indices[i++] = bottomStart + ringsPerHemisphere*(slices+1) + k;
    }

    UploadMesh(&mesh, false);
    return mesh;
}

int main(void) {
    InitWindow(800, 600, "ODE + Raylib: Capsule Character Controller");
    SetTargetFPS(60);

    Camera3D camera = {0};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = (Vector3){0, 1, 0};

    // Models
    cubeModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    // capsuleModel = LoadModelFromMesh(GenMeshCapsule(0.5f, 1.0f, 16, 16));
    capsuleModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    
    // ODE Setup
    dInitODE();
    world = dWorldCreate();
    space = dHashSpaceCreate(0);
    contactgroup = dJointGroupCreate(0);
    dWorldSetGravity(world, 0, -9.81f, 0);

    // Floor
    floorGeom = dCreateBox(space, 10.0f, 1.0f, 10.0f);
    dGeomSetPosition(floorGeom, 0, -0.5f, 0);

    // Initial positions
    Vector3 p1 = {-1.0f, 1.5f, 0.0f};
    Vector3 p2 = {-2.0f, 3.5f, 0.0f};
    Vector3 pc = { 0.0f, 2.5f, 0.0f};

    // Cube 1
    cube1Body = dBodyCreate(world);
    cube1Geom = dCreateBox(space, 1,1,1);
    dMass m; dMassSetBox(&m, 1.0f, 1,1,1);
    dBodySetMass(cube1Body, &m);
    dGeomSetBody(cube1Geom, cube1Body);
    dBodySetPosition(cube1Body, p1.x, p1.y, p1.z);

    // Cube 2
    cube2Body = dBodyCreate(world);
    cube2Geom = dCreateBox(space, 1,1,1);
    dMassSetBox(&m, 1.0f, 1,1,1);
    dBodySetMass(cube2Body, &m);
    dGeomSetBody(cube2Geom, cube2Body);
    dBodySetPosition(cube2Body, p2.x, p2.y, p2.z);

    // Capsule
    capsuleBody = dBodyCreate(world);
    capsuleGeom = dCreateCapsule(space, 0.5f, 1.0f);
    dMassSetCapsule(&m, 1.0f, 2, 0.5f, 1.0f);
    dBodySetMass(capsuleBody, &m);
    dGeomSetBody(capsuleGeom, capsuleBody);
    dQuaternion q; dQFromAxisAndAngle(q, 1,0,0, PI/2);
    dBodySetQuaternion(capsuleBody, q);
    dBodySetPosition(capsuleBody, pc.x, pc.y, pc.z);
    dBodySetMaxAngularSpeed(capsuleBody, 0.0f);

    printf("init loop\n");

    while (!WindowShouldClose()) {
        // === ODE STEP ===
        dSpaceCollide(space, 0, &nearCallback);
        dWorldQuickStep(world, 0.016f);
        dJointGroupEmpty(contactgroup);

        // === INPUT ===
        if (IsKeyPressed(KEY_ONE)) capsuleMovementEnabled = !capsuleMovementEnabled;
        if (IsKeyPressed(KEY_TAB)) {
            cameraControlEnabled = !cameraControlEnabled;
            cameraControlEnabled ? DisableCursor() : EnableCursor();
        }

        // === MOUSE LOOK ===
        if (cameraControlEnabled) {
            Vector2 delta = GetMouseDelta();
            cameraAngleX += delta.x * 0.3f; // Mouse right → camera right
            cameraAngleY -= delta.y * 0.3f;
            cameraAngleX = NormalizeAngleDegrees(cameraAngleX);
            cameraAngleY = Clamp(cameraAngleY, -89.0f, 89.0f);
        }

        // === RESET ===
        if (IsKeyPressed(KEY_R)) {
            dBodySetPosition(cube1Body, p1.x, p1.y, p1.z);
            dBodySetLinearVel(cube1Body, 0,0,0); dBodySetAngularVel(cube1Body, 0,0,0);
            dQuaternion id = {1,0,0,0}; dBodySetQuaternion(cube1Body, id);

            dBodySetPosition(cube2Body, p2.x, p2.y, p2.z);
            dBodySetLinearVel(cube2Body, 0,0,0); dBodySetAngularVel(cube2Body, 0,0,0);
            dBodySetQuaternion(cube2Body, id);

            dBodySetPosition(capsuleBody, pc.x, pc.y, pc.z);
            dBodySetLinearVel(capsuleBody, 0,0,0); dBodySetAngularVel(capsuleBody, 0,0,0);
            dQFromAxisAndAngle(q, 1,0,0, PI/2); dBodySetQuaternion(capsuleBody, q);
        }

        // === DIRECTION VECTORS ===
        float yaw = cameraAngleX * DEG2RAD;
        float pitch = cameraAngleY * DEG2RAD;

        Vector3 forward = {
            cosf(pitch) * cosf(yaw),
            sinf(pitch),
            cosf(pitch) * sinf(yaw)
        };

        Vector3 forwardXZ = { cosf(yaw), 0.0f, sinf(yaw) };
        Vector3 rightXZ   = { -sinf(yaw), 0.0f, cosf(yaw) };

        // === CAPSULE MOVEMENT ===
        if (capsuleMovementEnabled) {
            Vector3 force = {0};

            if (IsKeyDown(KEY_S)) force = Vector3Add(force, Vector3Scale(rightXZ, -20.0f));
            if (IsKeyDown(KEY_W)) force = Vector3Add(force, Vector3Scale(rightXZ,  20.0f));

            if (IsKeyDown(KEY_A)) force = Vector3Add(force, Vector3Scale(forwardXZ, 20.0f));
            if (IsKeyDown(KEY_D)) force = Vector3Add(force, Vector3Scale(forwardXZ, -20.0f));

            // if (IsKeyDown(KEY_W)) force = Vector3Add(force, Vector3Scale(forwardXZ, 20.0f));
            // if (IsKeyDown(KEY_S)) force = Vector3Add(force, Vector3Scale(forwardXZ, -20.0f));
            // if (IsKeyDown(KEY_A)) force = Vector3Add(force, Vector3Scale(rightXZ, -20.0f));
            // if (IsKeyDown(KEY_D)) force = Vector3Add(force, Vector3Scale(rightXZ,  20.0f));

            if (IsKeyPressed(KEY_SPACE)) {
                const dReal *pos = dBodyGetPosition(capsuleBody);
                if (pos[1] < 1.1f) dBodyAddForce(capsuleBody, 0, 500.0f, 0);
            }

            // dBodyAddForce(capsuleBody, force.x, force.y, force.z);
            dBodyAddForce(capsuleBody, force.x, 0.0f, force.z);
        }

        // === CAMERA FOLLOW (Third-Person) ===
        if (capsuleMovementEnabled) {
            const dReal *pos = dBodyGetPosition(capsuleBody);
            Vector3 capsulePos = { (float)pos[0], (float)pos[1], (float)pos[2] };

            // Offset: behind and above
            Vector3 localOffset = { 0.0f, 3.0f, -5.0f };  // -Z = behind
            Vector3 worldOffset = {
                localOffset.x * cosf(yaw) - localOffset.z * sinf(yaw),
                localOffset.y,
                localOffset.x * sinf(yaw) + localOffset.z * cosf(yaw)
            };

            Vector3 targetPos = Vector3Add(capsulePos, worldOffset);
            float t = 8.0f * GetFrameTime();
            camera.position = Vector3Lerp(camera.position, targetPos, t);
            camera.target = Vector3Add(capsulePos, (Vector3){0, 0.5f, 0});
        }
        // === FREE CAMERA ===
        else if (cameraControlEnabled) {
            float speed = 5.0f * GetFrameTime();
            if (IsKeyDown(KEY_W)) camera.position = Vector3Add(camera.position, Vector3Scale(forward, speed));
            if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, speed));
            if (IsKeyDown(KEY_A)) camera.position = Vector3Subtract(camera.position, Vector3Scale((Vector3){rightXZ.x, 0, rightXZ.z}, speed));
            if (IsKeyDown(KEY_D)) camera.position = Vector3Add(camera.position, Vector3Scale((Vector3){rightXZ.x, 0, rightXZ.z}, speed));
            if (IsKeyDown(KEY_SPACE)) camera.position.y += speed;
            if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.y -= speed;
            camera.target = Vector3Add(camera.position, forward);
        } else {
            camera.target = Vector3Add(camera.position, forward);
        }

        // === RENDER ===
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);

        DrawCube((Vector3){0, -0.5f, 0}, 10, 1, 10, GRAY);
        DrawCubeWires((Vector3){0, -0.5f, 0}, 10, 1, 10, BLACK);

        // Cube 1
        const dReal *p = dBodyGetPosition(cube1Body);
        const dReal *q = dBodyGetQuaternion(cube1Body);
        Matrix m = ODEQuaternionToMatrix((dQuaternion){q[0],q[1],q[2],q[3]});
        m.m12 = p[0]; m.m13 = p[1]; m.m14 = p[2];
        cubeModel.transform = m;
        DrawModel(cubeModel, (Vector3){0}, 1.0f, RED);
        DrawModelWires(cubeModel, (Vector3){0}, 1.0f, BLACK);

        // Cube 2
        p = dBodyGetPosition(cube2Body);
        q = dBodyGetQuaternion(cube2Body);
        m = ODEQuaternionToMatrix((dQuaternion){q[0],q[1],q[2],q[3]});
        m.m12 = p[0]; m.m13 = p[1]; m.m14 = p[2];
        cubeModel.transform = m;
        DrawModel(cubeModel, (Vector3){0}, 1.0f, BLUE);
        DrawModelWires(cubeModel, (Vector3){0}, 1.0f, BLACK);

        // Capsule
        p = dBodyGetPosition(capsuleBody);
        q = dBodyGetQuaternion(capsuleBody);
        m = ODEQuaternionToMatrix((dQuaternion){q[0],q[1],q[2],q[3]});
        m.m12 = p[0]; m.m13 = p[1]; m.m14 = p[2];
        capsuleModel.transform = m;
        DrawModel(capsuleModel, (Vector3){0}, 1.0f, YELLOW);
        DrawModelWires(capsuleModel, (Vector3){0}, 1.0f, BLACK);

        EndMode3D();

        // UI
        DrawText("R - Reset | 1 - Toggle Movement | TAB - Mouse Look", 10, 10, 20, BLACK);
        DrawText(TextFormat("Movement: %s", capsuleMovementEnabled ? "ON" : "OFF"), 10, 35, 20, capsuleMovementEnabled ? GREEN : DARKGRAY);
        DrawText(TextFormat("Yaw: %.1f | Pitch: %.1f", cameraAngleX, cameraAngleY), 10, 60, 20, BLACK);

        EndDrawing();
    }
    printf("clean up\n");

    // Cleanup
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
    printf("cubeModel..\n");
    UnloadModel(cubeModel); 
    UnloadModel(capsuleModel);
    CloseWindow();
    printf("finished..\n");
    return 0;
}