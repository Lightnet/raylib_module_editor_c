#include "raylib.h"
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
Model capsuleModel2; //test

// Global variable for the fixed joint
dJointID capsuleFixedJoint;

// Global variables for camera
bool cameraControlEnabled = false; // Toggle for mouse controls
float cameraAngleX = 233.0f; // Horizontal angle (degrees, yaw)
float cameraAngleY = -33.0f; // Vertical angle (degrees, pitch)



Mesh GenMeshCapsuleYAxis(float radius, float height, int slices, int rings) {
    Mesh mesh = { 0 };
    float halfHeight = height / 2.0f;
    int ringsPerHemisphere = rings / 2; // Rings per hemisphere, excluding poles
    // Vertex count: 1 pole + ringsPerHemisphere * (slices + 1) for top hemisphere,
    // + (ringsPerHemisphere + 1) * (slices + 1) for bottom hemisphere,
    // + 2 cylinder rings, + 1 bottom pole
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

    // Top pole (single vertex at y = halfHeight + radius)
    printf("Top Pole Vertex:\n");
    mesh.vertices[v * 3 + 0] = 0.0f;
    mesh.vertices[v * 3 + 1] = halfHeight + radius; // y-axis
    mesh.vertices[v * 3 + 2] = 0.0f;
    mesh.normals[v * 3 + 0] = 0.0f;
    mesh.normals[v * 3 + 1] = 1.0f; // Normal along y
    mesh.normals[v * 3 + 2] = 0.0f;
    mesh.texcoords[v * 2 + 0] = 0.5f;
    mesh.texcoords[v * 2 + 1] = 0.0f;
    printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, mesh.vertices[v * 3 + 0], mesh.vertices[v * 3 + 1], mesh.vertices[v * 3 + 2]);
    v++;

    // Top hemisphere (y from halfHeight to halfHeight + radius)
    printf("\nTop Hemisphere Vertices:\n");
    for (int j = 1; j <= ringsPerHemisphere; j++) {
        float phi = (float)j * PI / 2.0f / ringsPerHemisphere; // 0 to PI/2
        for (int k = 0; k <= slices; k++) {
            float theta = (float)k * 2.0f * PI / slices;
            float sinPhi = sinf(phi);
            float x = radius * sinPhi * cosf(theta);
            float y = radius * cosf(phi) + halfHeight; // y-axis
            float z = radius * sinPhi * sinf(theta);
            mesh.vertices[v * 3 + 0] = x;
            mesh.vertices[v * 3 + 1] = y;
            mesh.vertices[v * 3 + 2] = z;
            mesh.normals[v * 3 + 0] = x / radius;
            mesh.normals[v * 3 + 1] = (y - halfHeight) / radius; // Normal adjusted for y-axis
            mesh.normals[v * 3 + 2] = z / radius;
            mesh.texcoords[v * 2 + 0] = (float)k / slices;
            mesh.texcoords[v * 2 + 1] = (float)j / (ringsPerHemisphere * 2);
            printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, x, y, z);
            v++;
        }
    }

    // Cylinder top ring (at y = halfHeight)
    printf("\nCylinder Top Ring Vertices:\n");
    for (int k = 0; k <= slices; k++) {
        float theta = (float)k * 2.0f * PI / slices;
        float x = radius * cosf(theta);
        float y = halfHeight; // y-axis
        float z = radius * sinf(theta);
        mesh.vertices[v * 3 + 0] = x;
        mesh.vertices[v * 3 + 1] = y;
        mesh.vertices[v * 3 + 2] = z;
        mesh.normals[v * 3 + 0] = x / radius;
        mesh.normals[v * 3 + 1] = 0.0f; // Normal perpendicular to y-axis
        mesh.normals[v * 3 + 2] = z / radius;
        mesh.texcoords[v * 2 + 0] = (float)k / slices;
        mesh.texcoords[v * 2 + 1] = 0.5f;
        printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, x, y, z);
        v++;
    }

    // Cylinder bottom ring (at y = -halfHeight)
    printf("\nCylinder Bottom Ring Vertices:\n");
    for (int k = 0; k <= slices; k++) {
        float theta = (float)k * 2.0f * PI / slices;
        float x = radius * cosf(theta);
        float y = -halfHeight; // y-axis
        float z = radius * sinf(theta);
        mesh.vertices[v * 3 + 0] = x;
        mesh.vertices[v * 3 + 1] = y;
        mesh.vertices[v * 3 + 2] = z;
        mesh.normals[v * 3 + 0] = x / radius;
        mesh.normals[v * 3 + 1] = 0.0f;
        mesh.normals[v * 3 + 2] = z / radius;
        mesh.texcoords[v * 2 + 0] = (float)k / slices;
        mesh.texcoords[v * 2 + 1] = 0.5f;
        printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, x, y, z);
        v++;
    }

    // Bottom hemisphere (y from -halfHeight to -halfHeight - radius)
    printf("\nBottom Hemisphere Vertices:\n");
    for (int j = 0; j <= ringsPerHemisphere; j++) {
        float phi = PI / 2.0f + (float)j * PI / 2.0f / ringsPerHemisphere; // PI/2 to PI
        for (int k = 0; k <= slices; k++) {
            float theta = (float)k * 2.0f * PI / slices;
            float sinPhi = sinf(phi);
            float x = radius * sinPhi * cosf(theta);
            float y = radius * cosf(phi) - halfHeight; // y-axis
            float z = radius * sinPhi * sinf(theta);
            mesh.vertices[v * 3 + 0] = x;
            mesh.vertices[v * 3 + 1] = y;
            mesh.vertices[v * 3 + 2] = z;
            mesh.normals[v * 3 + 0] = x / radius;
            mesh.normals[v * 3 + 1] = (y + halfHeight) / radius; // Normal adjusted
            mesh.normals[v * 3 + 2] = z / radius;
            mesh.texcoords[v * 2 + 0] = (float)k / slices;
            mesh.texcoords[v * 2 + 1] = (float)(j + ringsPerHemisphere) / (ringsPerHemisphere * 2);
            printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, x, y, z);
            v++;
        }
    }

    // Bottom pole (single vertex at y = -halfHeight - radius)
    printf("\nBottom Pole Vertex:\n");
    mesh.vertices[v * 3 + 0] = 0.0f;
    mesh.vertices[v * 3 + 1] = -halfHeight - radius; // y-axis
    mesh.vertices[v * 3 + 2] = 0.0f;
    mesh.normals[v * 3 + 0] = 0.0f;
    mesh.normals[v * 3 + 1] = -1.0f; // Normal along negative y
    mesh.normals[v * 3 + 2] = 0.0f;
    mesh.texcoords[v * 2 + 0] = 0.5f;
    mesh.texcoords[v * 2 + 1] = 1.0f;
    printf("Vertex %d: (%.4f, %.4f, %.4f)\n", v, mesh.vertices[v * 3 + 0], mesh.vertices[v * 3 + 1], mesh.vertices[v * 3 + 2]);
    v++;

    // Indices with corrected winding order (counter-clockwise for outward normals)
    int topHemisphereStart = 1;
    printf("\nTop Hemisphere Indices:\n");
    // Top pole triangles (connect pole to first ring, reversed winding)
    for (int k = 0; k < slices; k++) {
        int v0 = 0; // Top pole
        int v1 = topHemisphereStart + k;
        int v2 = topHemisphereStart + (k + 1) % (slices + 1);
        mesh.indices[i++] = v0;
        mesh.indices[i++] = v2; // Swap v1 and v2 for counter-clockwise
        mesh.indices[i++] = v1;
        printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v0, v2, v1);
    }
    // Top hemisphere quads
    for (int j = 0; j < ringsPerHemisphere - 1; j++) {
        for (int k = 0; k < slices; k++) {
            int v0 = topHemisphereStart + j * (slices + 1) + k;
            int v1 = v0 + 1;
            int v2 = topHemisphereStart + (j + 1) * (slices + 1) + k;
            int v3 = v2 + 1;
            // First triangle
            mesh.indices[i++] = v0;
            mesh.indices[i++] = v1; // Reversed order
            mesh.indices[i++] = v2;
            // Second triangle
            mesh.indices[i++] = v1;
            mesh.indices[i++] = v3; // Reversed order
            mesh.indices[i++] = v2;
            printf("Triangle %d: (%d, %d, %d)\n", i/3-2, v0, v1, v2);
            printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v1, v3, v2);
        }
    }

    // Cylinder indices
    int topRingOffset = topHemisphereStart + ringsPerHemisphere * (slices + 1);
    int bottomRingOffset = topRingOffset + (slices + 1);
    printf("\nCylinder Indices:\n");
    for (int k = 0; k < slices; k++) {
        int v0 = topRingOffset + k;
        int v1 = v0 + 1;
        int v2 = bottomRingOffset + k;
        int v3 = v2 + 1;
        // First triangle
        mesh.indices[i++] = v0;
        mesh.indices[i++] = v1; // Reversed order
        mesh.indices[i++] = v2;
        // Second triangle
        mesh.indices[i++] = v1;
        mesh.indices[i++] = v3; // Reversed order
        mesh.indices[i++] = v2;
        printf("Triangle %d: (%d, %d, %d)\n", i/3-2, v0, v1, v2);
        printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v1, v3, v2);
    }

    // Bottom hemisphere indices
    int bottomHemisphereOffset = bottomRingOffset + (slices + 1);
    int bottomPole = bottomHemisphereOffset + ringsPerHemisphere * (slices + 1);
    printf("\nBottom Hemisphere Indices:\n");
    for (int j = 0; j < ringsPerHemisphere; j++) {
        for (int k = 0; k < slices; k++) {
            int v0 = bottomHemisphereOffset + j * (slices + 1) + k;
            int v1 = v0 + 1;
            int v2 = bottomHemisphereOffset + (j + 1) * (slices + 1) + k;
            int v3 = v2 + 1;
            // First triangle
            mesh.indices[i++] = v0;
            mesh.indices[i++] = v1; // Reversed order
            mesh.indices[i++] = v2;
            // Second triangle
            mesh.indices[i++] = v1;
            mesh.indices[i++] = v3; // Reversed order
            mesh.indices[i++] = v2;
            printf("Triangle %d: (%d, %d, %d)\n", i/3-2, v0, v1, v2);
            printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v1, v3, v2);
        }
    }
    // Bottom pole triangles (reversed winding)
    for (int k = 0; k < slices; k++) {
        int v0 = bottomPole;
        int v1 = bottomHemisphereOffset + ringsPerHemisphere * (slices + 1) + k;
        int v2 = bottomHemisphereOffset + ringsPerHemisphere * (slices + 1) + (k + 1) % (slices + 1);
        mesh.indices[i++] = v0;
        mesh.indices[i++] = v1; // Reversed from original
        mesh.indices[i++] = v2;
        printf("Triangle %d: (%d, %d, %d)\n", i/3-1, v0, v1, v2);
    }

    // Upload mesh data to GPU
    UploadMesh(&mesh, false);
    return mesh;
}



// Contact callback for collision handling
void nearCallback(void *data, dGeomID o1, dGeomID o2) {
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    dContact contact;
    contact.surface.mode = dContactBounce;
    // contact.surface.mu = dInfinity; // Friction
    contact.surface.mu = 1.0f; // Friction
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
    dBodySetPosition(cube1Body, initialCube1Pos.x, initialCube1Pos.y, initialCube1Pos.z); // Lower cube initial position

    // Create second dynamic cube (stacked on top)
    cube2Body = dBodyCreate(world);
    cube2Geom = dCreateBox(space, 1.0f, 1.0f, 1.0f);
    dMass mass2;
    dMassSetBox(&mass2, 1.0f, 1.0f, 1.0f, 1.0f); // Density = 1.0
    dBodySetMass(cube2Body, &mass2);
    dGeomSetBody(cube2Geom, cube2Body);
    dBodySetPosition(cube2Body, initialCube2Pos.x, initialCube2Pos.y, initialCube2Pos.z); // Upper cube initial position

    // Create capsule model
    Mesh capsuleMesh = GenMeshCapsule(0.5f, 1.0f, 8, 8); // Radius 0.5, cylinder height 1.0
    capsuleModel = LoadModelFromMesh(capsuleMesh);

    // Create capsule in ODE
    capsuleBody = dBodyCreate(world);
    capsuleGeom = dCreateCapsule(space, 0.5f, 1.0f); // Radius 0.5, cylinder length 1.0
    dMass massCapsule;
    // direction (1=x, 2=y, 3=z) // https://ode.org/wiki/index.php/Manual
    // dMassSetCapsule(&massCapsule, 1.0f, 3, 0.5f, 1.0f); // Density = 1.0, direction along z-axis (3)
    dMassSetCapsule(&massCapsule, 1.0f, 2, 0.5f, 1.0f); // Density = 1.0, direction along y-axis (2)
    // dMassSetCapsuleTotal(&massCapsule, 1.0f, 2, 0.5f, 1.0f);
    dBodySetMass(capsuleBody, &massCapsule);
    dGeomSetBody(capsuleGeom, capsuleBody);
    // default
    // dQuaternion identityCapsule = { 1.0f, 0.0f, 0.0f, 0.0f };
    // dBodySetQuaternion(capsuleBody, identityCapsule);

    // Set rotation to align z-axis with y-axis (90 degrees around x-axis)
    // dQuaternion quat;
    // float angle = PI / 2.0f; // 90 degrees in radians
    // dQFromAxisAndAngle(quat, 1.0f, 0.0f, 0.0f, angle); // Rotate around x-axis
    // dBodySetQuaternion(capsuleBody, quat);

    //Set rotation to align z-axis with y-axis (90 degrees around x-axis)
    dQuaternion yAxisQuat;
    dQFromAxisAndAngle(yAxisQuat, 1.0f, 0.0f, 0.0f, PI / 2.0f); // Rotate z to y
    dBodySetQuaternion(capsuleBody, yAxisQuat);

    dBodySetPosition(capsuleBody, initialCapsulePos.x, initialCapsulePos.y, initialCapsulePos.z); // Position capsule to the side
    dBodySetMaxAngularSpeed(capsuleBody, 0.0);


    // Optional: Lock rotation with fixed joint
    // capsuleFixedJoint = dJointCreateFixed(world, 0);
    // dJointAttach(capsuleFixedJoint, capsuleBody, 0);
    // dJointSetFixed(capsuleFixedJoint);

    // Create capsule model
    Mesh capsuleMesh2 = GenMeshCapsuleYAxis(0.5f, 1.0f, 8, 8); // Radius 0.5, cylinder height 1.0
    capsuleModel2 = LoadModelFromMesh(capsuleMesh2);



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

            // Reset capsule
            dBodySetPosition(capsuleBody, initialCapsulePos.x, initialCapsulePos.y, initialCapsulePos.z);
            dBodySetLinearVel(capsuleBody, 0, 0, 0);
            dBodySetAngularVel(capsuleBody, 0, 0, 0);


            dQFromAxisAndAngle(yAxisQuat, 1.0f, 0.0f, 0.0f, PI / 2.0f);
            dBodySetQuaternion(capsuleBody, yAxisQuat);


            // dQuaternion identityCapsule = { 1.0f, 0.0f, 0.0f, 0.0f };
            // dBodySetQuaternion(capsuleBody, identityCapsule);

            // dQuaternion quat;
            // float angle = PI / 2.0f; // 90 degrees in radians
            // dQFromAxisAndAngle(quat, 1.0f, 0.0f, 0.0f, angle); // Rotate around x-axis
            // dBodySetQuaternion(capsuleBody, quat);
            
        }


        // Toggle camera controls with Tab key
        if (IsKeyPressed(KEY_TAB)) {
            cameraControlEnabled = !cameraControlEnabled;
            if (cameraControlEnabled) {
                DisableCursor(); // Lock cursor for mouse control
            } else {
                EnableCursor(); // Unlock cursor
            }
        }

        // Update camera if controls are enabled
        if (cameraControlEnabled) {
            // Mouse rotation (yaw and pitch)
            Vector2 mouseDelta = GetMouseDelta();
            cameraAngleX -= mouseDelta.x * 0.3f * -1; // Adjust sensitivity
            cameraAngleX = NormalizeAngleDegrees(cameraAngleX);
            cameraAngleY -= mouseDelta.y * 0.3f;
            // Clamp vertical angle to avoid flipping
            if (cameraAngleY > 89.0f) cameraAngleY = 89.0f;
            if (cameraAngleY < -89.0f) cameraAngleY = -89.0f;

            // Compute forward and right vectors based on yaw and pitch
            float yawRad = cameraAngleX * DEG2RAD;
            float pitchRad = cameraAngleY * DEG2RAD;
            Vector3 forward = {
                cosf(pitchRad) * cosf(yawRad),
                sinf(pitchRad),
                cosf(pitchRad) * sinf(yawRad)
            };
            // Normalize forward vector
            float len = sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
            if (len > 0) {
                forward.x /= len;
                forward.y /= len;
                forward.z /= len;
            }

            // Compute right vector (perpendicular to forward and up)
            Vector3 up = { 0.0f, 1.0f, 0.0f }; // World up vector
            Vector3 right = {
                -sinf(yawRad),
                0.0f,
                cosf(yawRad)
            };
            // Normalize right vector
            len = sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);
            if (len > 0) {
                right.x /= len;
                right.y /= len;
                right.z /= len;
            }

            // Camera movement speed
            float moveSpeed = 5.0f * GetFrameTime(); // Speed adjusted for frame rate (e.g., 5 units/second)

            // Forward/backward movement (W/S keys)
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

            // Strafe left/right (A/D keys)
            if (IsKeyDown(KEY_A)) {
                camera.position.x -= right.x * moveSpeed;
                camera.position.y -= right.y * moveSpeed;
                camera.position.z -= right.z * moveSpeed;
            }
            if (IsKeyDown(KEY_D)) {
                camera.position.x += right.x * moveSpeed;
                camera.position.y += right.y * moveSpeed;
                camera.position.z += right.z * moveSpeed;
            }

            // Up/down movement (Space/Shift keys)
            if (IsKeyDown(KEY_SPACE)) {
                camera.position.y += moveSpeed;
            }
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                camera.position.y -= moveSpeed;
            }

            // Update camera target based on forward direction
            camera.target.x = camera.position.x + forward.x;
            camera.target.y = camera.position.y + forward.y;
            camera.target.z = camera.position.z + forward.z;
        }


        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // Draw floor
        DrawCube((Vector3){ 0.0f, -0.5f, 0.0f }, 10.0f, 1.0f, 10.0f, GRAY);
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

        // Draw capsule with wireframe
        const dReal *capsulePos = dBodyGetPosition(capsuleBody);
        const dReal *capsuleQuatPtr = dBodyGetQuaternion(capsuleBody);
        dQuaternion capsuleQuat = { capsuleQuatPtr[0], capsuleQuatPtr[1], capsuleQuatPtr[2], capsuleQuatPtr[3] };
        Matrix capsuleRotMatrix = QuaternionToMatrix(capsuleQuat);
        capsuleRotMatrix.m12 = (float)capsulePos[0];
        capsuleRotMatrix.m13 = (float)capsulePos[1];
        capsuleRotMatrix.m14 = (float)capsulePos[2];
        capsuleModel.transform = capsuleRotMatrix;
        DrawModel(capsuleModel, (Vector3){0, 0, 0}, 1.0f, YELLOW); // Solid model (optional)
        DrawModelWires(capsuleModel, (Vector3){0, 0, 0}, 1.0f, BLACK); // Wireframe for rotation check


        DrawModel(capsuleModel2, (Vector3){0, 0, 0}, 1.0f, BLUE); // Solid model (optional)
        DrawModelWires(capsuleModel2, (Vector3){0, 0, 0}, 1.0f, BLACK); // Wireframe for rotation check

        EndMode3D();

        // Draw instructions
        DrawText("Press 'R' to reset cubes", 10, 10, 20, BLACK);
        DrawText("Press 'TAB' to toggle camera controls", 10, 30, 20, BLACK);
        DrawText(cameraControlEnabled ? "Camera: Mouse active" : "Camera: Mouse inactive", 10, 50, 20, BLACK);
        DrawText(TextFormat("Angle X: %0.02f", cameraAngleX), 10, 70, 20, BLACK);
        DrawText(TextFormat("Angle Y: %0.02f", cameraAngleY), 10, 90, 20, BLACK);

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
    CloseWindow();

    return 0;
}