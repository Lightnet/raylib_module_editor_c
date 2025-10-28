/*********************************************************************
 *  ODE + raylib – two cubes with collision filtering
 *  Player (red)  : category 0x0001   collide 0xFFFE
 *  Enemy  (blue) : category 0x0002   collide 0xFFFD
 *  Ground        : category 0x0004   collide 0xFFFF
 *
 *  Player & Enemy never generate contact joints with each other.
 *********************************************************************/

#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "ode/ode.h"

/* --------------------------------------------------------------- */
/*  Physics objects                                                 */
dWorldID      world;
dSpaceID      space;
dGeomID       ground;
dJointGroupID contact_group;

/* Two cubes */
typedef enum { TYPE_PLAYER, TYPE_ENEMY } ObjType;

typedef struct {
    ObjType type;
    dBodyID body;
    dGeomID geom;
    Color   color;
} Cube;

Cube player = {0};
Cube enemy  = {0};

/* --------------------------------------------------------------- */
/*  Collision callback – filters with category/collide bits        */
static void nearCallback(void *data, dGeomID o1, dGeomID o2)
{
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);

    // Skip if bodies are connected (e.g. via joint)
    if (b1 && b2 && dAreConnected(b1, b2)) return;

    // --- CRITICAL: Manual category/collide bit check BEFORE dCollide ---
    unsigned long cat1 = dGeomGetCategoryBits(o1);
    unsigned long cat2 = dGeomGetCategoryBits(o2);
    unsigned long col1 = dGeomGetCollideBits(o1);
    unsigned long col2 = dGeomGetCollideBits(o2);

    if (!(cat1 & col2) || !(cat2 & col1)) {
        // Bits say: "do NOT collide" → skip entirely
        return;
    }

    // --- Only reach here if collision is allowed ---
    const int MAX_CONTACTS = 8;
    dContact contacts[MAX_CONTACTS];

    int numc = dCollide(o1, o2, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));
    if (numc == 0) return;

    for (int i = 0; i < numc; ++i) {
        contacts[i].surface.mode = dContactBounce | dContactSoftCFM;
        contacts[i].surface.mu   = dInfinity;
        contacts[i].surface.bounce     = 0.5f;
        contacts[i].surface.bounce_vel = 0.1f;
        contacts[i].surface.soft_cfm   = 0.01f;

        dJointID c = dJointCreateContact(world, contact_group, &contacts[i]);
        dJointAttach(c, b1, b2);
    }
}

/* --------------------------------------------------------------- */
void resetCube(Cube *c)
{
    // float x = (float)GetRandomValue(-5, 5);
    // float z = (float)GetRandomValue(-5, 5);
    // float y = (float)GetRandomValue(5, 15);

    float x = 0.0f;
    float z = 0.0f;
    float y = 0.0f;
    dBodySetPosition(c->body, x, y, z);
    dBodySetLinearVel(c->body, 0, 0, 0);
    dBodySetAngularVel(c->body, 0, 0, 0);
}

/* --------------------------------------------------------------- */
Matrix odeToRLTransform(dBodyID body)
{
    const dReal *pos = dBodyGetPosition(body);
    const dReal *rot = dBodyGetRotation(body);

    Matrix rotM = {
        (float)rot[0], (float)rot[1], (float)rot[2], 0,
        (float)rot[4], (float)rot[5], (float)rot[6], 0,
        (float)rot[8], (float)rot[9], (float)rot[10],0,
        0,0,0,1
    };
    Matrix transM = MatrixTranslate((float)pos[0], (float)pos[1], (float)pos[2]);
    return MatrixMultiply(rotM, transM);
}

/* --------------------------------------------------------------- */
void getYawPitchRoll(const dReal *rot, float *yaw, float *pitch, float *roll)
{
    float r11 = rot[0], r12 = rot[4], r13 = rot[8];
    float r21 = rot[1], r23 = rot[9];
    float r31 = rot[2], r32 = rot[6], r33 = rot[10];

    *pitch = atan2f(-r31, sqrtf(r11*r11 + r21*r21)) * RAD2DEG;
    *yaw   = atan2f(r21, r11) * RAD2DEG;
    *roll  = atan2f(r32, r33) * RAD2DEG;
}

/* --------------------------------------------------------------- */
int main(void)
{
    /* ------------------- ODE init ------------------- */
    dInitODE();
    world = dWorldCreate();
    space = dHashSpaceCreate(0);
    contact_group = dJointGroupCreate(0);
    dWorldSetGravity(world, 0, -9.81f, 0);

    /* Ground plane – always collides with everything */
    ground = dCreatePlane(space, 0, 1, 0, 0);
    dGeomSetCategoryBits(ground, 0x0004);
    dGeomSetCollideBits (ground, 0xFFFF);

    const float cubeSize = 1.0f;

    /* ------------------- PLAYER (red) ------------------- */
    player.type  = TYPE_PLAYER;
    player.color = RED;
    player.body  = dBodyCreate(world);
    player.geom  = dCreateBox(space, cubeSize, cubeSize, cubeSize);
    dGeomSetBody(player.geom, player.body);
    dMass m; dMassSetBox(&m, 1.0f, cubeSize, cubeSize, cubeSize);
    dBodySetMass(player.body, &m);
    dGeomSetCategoryBits(player.geom, 0x0001);   // I am a player
    // // dGeomSetCollideBits (player.geom, 0xFFFE);   // collide with everything except enemy (bit 1)
    dGeomSetCollideBits(player.geom, 0x0004);  // Only collide with ground
    resetCube(&player);

    /* ------------------- ENEMY (blue) ------------------- */
    enemy.type   = TYPE_ENEMY;
    enemy.color  = BLUE;
    enemy.body   = dBodyCreate(world);
    enemy.geom   = dCreateBox(space, cubeSize, cubeSize, cubeSize);
    dGeomSetBody(enemy.geom, enemy.body);
    dMassSetBox(&m, 1.0f, cubeSize, cubeSize, cubeSize);
    dBodySetMass(enemy.body, &m);
    dGeomSetCategoryBits(enemy.geom, 0x0002);    // I am an enemy
    // //  dGeomSetCollideBits (enemy.geom, 0xFFFD);    // collide with everything except player (bit 0)
    dGeomSetCollideBits(enemy.geom, 0x0004);   // Only collide with ground

    resetCube(&enemy);

    /* ------------------- raylib init ------------------- */
    InitWindow(800, 600, "ODE – Player (red) & Enemy (blue) – No collision between them");
    SetTargetFPS(60);

    Camera3D cam = {0};
    cam.position = (Vector3){0, 10, 10};
    cam.target   = (Vector3){0, 0, 0};
    cam.up       = (Vector3){0, 1, 0};
    cam.fovy     = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    Model cubeModel = LoadModelFromMesh(GenMeshCube(cubeSize, cubeSize, cubeSize));

    /* --------------------------------------------------- */
    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_R))
        {
            resetCube(&player);
            resetCube(&enemy);
        }

        /* ---- physics step ---- */
        dSpaceCollide(space, 0, &nearCallback);
        dWorldQuickStep(world, 1.0f/60.0f);
        dJointGroupEmpty(contact_group);

        /* ---- update raylib transforms ---- */
        cubeModel.transform = odeToRLTransform(player.body);
        float pyaw, ppitch, proll;
        getYawPitchRoll(dBodyGetRotation(player.body), &pyaw, &ppitch, &proll);

        Matrix enemyM = odeToRLTransform(enemy.body);
        float eyaw, epitch, eroll;
        getYawPitchRoll(dBodyGetRotation(enemy.body), &eyaw, &epitch, &eroll);

        /* ---- drawing ---- */
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(cam);

        DrawPlane((Vector3){0,0,0}, (Vector2){20,20}, LIGHTGRAY);

        // Player
        cubeModel.transform = odeToRLTransform(player.body);
        DrawModel(cubeModel, (Vector3){0,0,0}, 1.0f, player.color);
        DrawModelWires(cubeModel, (Vector3){0,0,0}, 1.0f, BLACK);

        // Enemy
        cubeModel.transform = enemyM;
        DrawModel(cubeModel, (Vector3){0,0,0}, 1.0f, enemy.color);
        DrawModelWires(cubeModel, (Vector3){0,0,0}, 1.0f, BLACK);

        EndMode3D();

        DrawFPS(10, 10);
        DrawText("Press R to reset both cubes", 10, 30, 20, DARKGRAY);
        DrawText(TextFormat("Player Pos: %.2f %.2f %.2f  YPR: %.0f %.0f %.0f",
                            dBodyGetPosition(player.body)[0],
                            dBodyGetPosition(player.body)[1],
                            dBodyGetPosition(player.body)[2],
                            pyaw, ppitch, proll), 10, 55, 18, DARKGRAY);
        DrawText(TextFormat("Enemy  Pos: %.2f %.2f %.2f  YPR: %.0f %.0f %.0f",
                            dBodyGetPosition(enemy.body)[0],
                            dBodyGetPosition(enemy.body)[1],
                            dBodyGetPosition(enemy.body)[2],
                            eyaw, epitch, eroll), 10, 80, 18, DARKGRAY);

        EndDrawing();
    }

    /* ------------------- cleanup ------------------- */
    UnloadModel(cubeModel);
    dJointGroupDestroy(contact_group);
    dSpaceDestroy(space);
    dWorldDestroy(world);
    dCloseODE();
    CloseWindow();
    return 0;
}