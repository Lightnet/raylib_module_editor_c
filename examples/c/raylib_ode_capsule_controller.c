// note that there damping which does not go straight.
// low force might off degree when doing forward( rotate 45 degree), due capsule still rotate?
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
        // the highter more friction. 
        // contact.surface.mu = 50.0f;
        // contact.surface.mu = 100.0f;
        contact.surface.mu = 5.0f;
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


int main(void) {
    InitWindow(800, 600, "ODE + Raylib: Capsule Character Controller");
    SetTargetFPS(60);

    Camera3D camera = {0};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = (Vector3){0, 1, 0};

    // Models
    cubeModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    capsuleModel = LoadModelFromMesh(GenMeshCapsule(0.5f, 1.0f, 8, 8));
    // capsuleModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    
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

    Vector3 playerPos = { 0.0f, 0.5f, 0.0f };
    const float camDistance = 8.0f;
    const float camPitch    = 45.0f * DEG2RAD;
    float camYaw = 0.0f;
    Vector2 mousePrev = { 0 };
    bool mousePressed = false;

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

        // Vector3 refPoint;

        // ---------------------------------------------------------------------
//  CAPSULE MOVEMENT + THIRD-PERSON CAMERA (ODE-driven)
// ---------------------------------------------------------------------
if (capsuleMovementEnabled) {

    /* ---------- 1. Mouse-look (rotate character yaw) ---------- */
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        mousePrev    = GetMousePosition();
        mousePressed = true;
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) mousePressed = false;

    if (mousePressed) {
        Vector2 now   = GetMousePosition();
        Vector2 delta = Vector2Subtract(now, mousePrev);
        camYaw       -= delta.x * 0.005f;               // <-- character yaw
        mousePrev    = now;
    }

    /* ---------- 2. Get capsule centre from ODE ---------- */
    const dReal *p = dBodyGetPosition(capsuleBody);
    Vector3 capsulePos = { (float)p[0], (float)p[1], (float)p[2] };

    /* ---------- 3. Third-person camera (behind & above) ---------- */
    const float camDistance = 8.0f;
    const float camPitch    = 45.0f * DEG2RAD;          // fixed pitch

    Vector3 camOffset = {
        camDistance * cosf(camPitch) * sinf(camYaw),   // X
        camDistance * sinf(camPitch),                 // Y
        camDistance * cosf(camPitch) * cosf(camYaw)    // Z
    };

    camera.position = Vector3Add(capsulePos, camOffset);
    camera.target   = capsulePos;                       // look at capsule centre

    /* ---------- 4. Build forward / right XZ vectors ---------- */
    Vector3 camForward = Vector3Subtract(camera.target, camera.position);
    camForward = Vector3Normalize(camForward);

    Vector3 forwardXZ = { camForward.x, 0.0f, camForward.z };
    forwardXZ = Vector3Normalize(forwardXZ);

    Vector3 rightXZ = Vector3CrossProduct(forwardXZ, (Vector3){0,1,0});
    rightXZ = Vector3Normalize(rightXZ);

    /* ---------- 5. WASD movement → force on capsule ---------- */
    // const float moveForce = 80.0f;                     // tweak to taste
    const float moveForce = 20.0f;                     // tweak to taste

    Vector3 wish = {0};
    if (IsKeyDown(KEY_W)) wish = Vector3Add(wish, forwardXZ);
    if (IsKeyDown(KEY_S)) wish = Vector3Subtract(wish, forwardXZ);
    if (IsKeyDown(KEY_A)) wish = Vector3Subtract(wish, rightXZ);
    if (IsKeyDown(KEY_D)) wish = Vector3Add(wish, rightXZ);


    if (Vector3LengthSqr(wish) > 0.0f) {
        // === LOCK ROTATION (no spin) ===
        dBodySetAngularVel(capsuleBody, 0, 0, 0);
        dBodySetMaxAngularSpeed(capsuleBody, 0.0f);


        // move base direction
        wish = Vector3Scale(Vector3Normalize(wish), moveForce);
        dBodyAddForce(capsuleBody, wish.x, 0.0f, wish.z);
    }

    /* ---------- 6. Simple jump (ground check) ---------- */
    if (IsKeyPressed(KEY_SPACE)) {
        if (p[1] < 1.1f)                     // capsule radius = 0.5 → ground ≈ 0.6
            dBodyAddForce(capsuleBody, 0.0f, 500.0f, 0.0f);
    }
}

        // // === CAMERA FOLLOW (Third-Person) ===
        // if (capsuleMovementEnabled) {
        //     const dReal *pos = dBodyGetPosition(capsuleBody);
        //     Vector3 capsulePos = { (float)pos[0], (float)pos[1], (float)pos[2] };

        //     // Offset: behind and above
        //     Vector3 localOffset = { 0.0f, 3.0f, -5.0f };  // -Z = behind
        //     Vector3 worldOffset = {
        //         localOffset.x * cosf(yaw) - localOffset.z * sinf(yaw),
        //         localOffset.y,
        //         localOffset.x * sinf(yaw) + localOffset.z * cosf(yaw)
        //     };

        //     Vector3 targetPos = Vector3Add(capsulePos, worldOffset);
        //     float t = 8.0f * GetFrameTime();
        //     camera.position = Vector3Lerp(camera.position, targetPos, t);
        //     camera.target = Vector3Add(capsulePos, (Vector3){0, 0.5f, 0});
        // }
        // // === FREE CAMERA ===
        // else if (cameraControlEnabled) {
        //     float speed = 5.0f * GetFrameTime();
        //     if (IsKeyDown(KEY_W)) camera.position = Vector3Add(camera.position, Vector3Scale(forward, speed));
        //     if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, speed));
        //     if (IsKeyDown(KEY_A)) camera.position = Vector3Subtract(camera.position, Vector3Scale((Vector3){rightXZ.x, 0, rightXZ.z}, speed));
        //     if (IsKeyDown(KEY_D)) camera.position = Vector3Add(camera.position, Vector3Scale((Vector3){rightXZ.x, 0, rightXZ.z}, speed));
        //     if (IsKeyDown(KEY_SPACE)) camera.position.y += speed;
        //     if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.y -= speed;
        //     camera.target = Vector3Add(camera.position, forward);
        // } else {
        //     camera.target = Vector3Add(camera.position, forward);
        // }

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

        DrawCube(playerPos,1.0f,1.0f,1.0f,BLUE);

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