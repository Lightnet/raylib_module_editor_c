/*********************************************************************
 *  ODE + raylib – FINAL: Large, Visible, Clean Trigger
 *  Ground: 0x0001 | Player: 0x0004 | Trigger: 0x0002
 *  ENTER/EXIT once, no spam, visible trigger
 *********************************************************************/

#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "ode/ode.h"

/* ---------- Physics ---------- */
dWorldID      world;
dSpaceID      space;
dGeomID       ground;
dJointGroupID contact_group;

/* ---------- Objects ---------- */
typedef enum { TYPE_PLAYER, TYPE_TRIGGER } ObjType;

typedef struct {
    ObjType type;
    dBodyID body;
    dGeomID geom;
    Color   color;
    float   size;
} Cube;

Cube player  = {0};
Cube trigger = {0};

/* ---------- Collision Context ---------- */
typedef struct {
    dGeomID player_geom;
    dGeomID trigger_geom;
} CollisionContext;
CollisionContext ctx;

/* ---------- Trigger State ---------- */
int playerInTrigger = 0;
int enteredPrinted  = 0;
int exitedPrinted   = 0;

/* ---------- Camera ---------- */
Camera3D camera = {0};

/* --------------------------------------------------------------- */
static void nearCallback(void *data, dGeomID o1, dGeomID o2)
{
    CollisionContext *c = (CollisionContext*)data;
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    if (b1 && b2 && dAreConnected(b1, b2)) return;

    unsigned long cat1 = dGeomGetCategoryBits(o1);
    unsigned long col1 = dGeomGetCollideBits(o1);
    unsigned long cat2 = dGeomGetCategoryBits(o2);
    unsigned long col2 = dGeomGetCollideBits(o2);

    // ENABLED: Broad-phase filter
    if (!(cat1 & col2) || !(cat2 & col1)) return;

    const int MAX_CONTACTS = 8;
    dContact contacts[MAX_CONTACTS];
    int n = dCollide(o1, o2, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));
    if (n == 0) return;

    int isTriggerPair = 0;
    if ((o1 == c->player_geom && o2 == c->trigger_geom) ||
        (o1 == c->trigger_geom && o2 == c->player_geom)) {
        isTriggerPair = 1;
        playerInTrigger = 1;
    }

    if (!isTriggerPair) {
        for (int i = 0; i < n; ++i) {
            contacts[i].surface.mode = dContactBounce | dContactSoftCFM;
            contacts[i].surface.mu   = dInfinity;
            contacts[i].surface.bounce     = 0.5f;
            contacts[i].surface.bounce_vel = 0.1f;
            contacts[i].surface.soft_cfm   = 0.01f;
            dJointID j = dJointCreateContact(world, contact_group, &contacts[i]);
            dJointAttach(j, b1, b2);
        }
    }
}

/* --------------------------------------------------------------- */
void resetPlayer(void)
{
    dBodySetPosition(player.body, 0, 15, 0);
    dBodySetLinearVel(player.body, 0, 0, 0);
    dBodySetAngularVel(player.body, 0, 0, 0);
    playerInTrigger = 0;
    enteredPrinted  = 0;
    exitedPrinted   = 0;
    printf("\n--- PLAYER RESET ---\n");
}

/* --------------------------------------------------------------- */
Matrix odeToRLTransform(dBodyID body)
{
    const dReal *p = dBodyGetPosition(body);
    const dReal *r = dBodyGetRotation(body);
    Matrix rot = {
        (float)r[0], (float)r[1], (float)r[2], 0,
        (float)r[4], (float)r[5], (float)r[6], 0,
        (float)r[8], (float)r[9],(float)r[10],0,
        0,0,0,1
    };
    Matrix tr = MatrixTranslate((float)p[0], (float)p[1], (float)p[2]);
    return MatrixMultiply(rot, tr);
}

/* --------------------------------------------------------------- */
void updateCamera(void)
{
    Vector2 mouse = GetMouseDelta();
    float sens = 0.003f;

    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    camera.position = Vector3RotateByAxisAngle(camera.position, (Vector3){0,1,0}, -mouse.x * sens);
    Vector3 right = Vector3CrossProduct(forward, (Vector3){0,1,0});
    camera.position = Vector3RotateByAxisAngle(camera.position, right, -mouse.y * sens);
    camera.target = Vector3Add(camera.position, forward);

    float speed = 20.0f * GetFrameTime();
    if (IsKeyDown(KEY_W)) camera.position = Vector3Add(camera.position, Vector3Scale(forward, speed));
    if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, speed));
    if (IsKeyDown(KEY_A)) camera.position = Vector3Subtract(camera.position, Vector3Scale(right, speed));
    if (IsKeyDown(KEY_D)) camera.position = Vector3Add(camera.position, Vector3Scale(right, speed));
    if (IsKeyDown(KEY_SPACE)) camera.position.y += speed;
    if (IsKeyDown(KEY_LEFT_CONTROL)) camera.position.y -= speed;
}

/* --------------------------------------------------------------- */
int main(void)
{
    dInitODE();
    world = dWorldCreate();
    space = dHashSpaceCreate(0);
    contact_group = dJointGroupCreate(0);
    dWorldSetGravity(world, 0, -9.81f, 0);

    /* GROUND: 0x0001 */
    ground = dCreatePlane(space, 0, 1, 0, 0);
    dGeomSetCategoryBits(ground, 0x0001);
    dGeomSetCollideBits (ground, 0x0004);

    const float playerSize  = 1.0f;
    const float triggerSize = 5.0f;  // LARGE, VISIBLE

    /* PLAYER: 0x0004 */
    player.type  = TYPE_PLAYER;
    player.color = RED;
    player.size  = playerSize;
    player.body  = dBodyCreate(world);
    player.geom  = dCreateBox(space, playerSize, playerSize, playerSize);
    dGeomSetBody(player.geom, player.body);
    dMass m; dMassSetBox(&m, 1.0f, playerSize, playerSize, playerSize);
    dBodySetMass(player.body, &m);
    dGeomSetCategoryBits(player.geom, 0x0004);
    dGeomSetCollideBits (player.geom, 0x0003);   // Ground + Trigger

    /* TRIGGER: 0x0002 */
    trigger.type  = TYPE_TRIGGER;
    trigger.color = BLUE;
    trigger.size  = triggerSize;
    trigger.body  = dBodyCreate(world);
    trigger.geom  = dCreateBox(space, triggerSize, triggerSize, triggerSize);
    dGeomSetBody(trigger.geom, trigger.body);
    dMassSetBox(&m, 1.0f, triggerSize, triggerSize, triggerSize);
    dBodySetMass(trigger.body, &m);
    dBodySetKinematic(trigger.body);
    dBodySetPosition(trigger.body, 0, 5, 0);  // Center at y=5
    dGeomSetCategoryBits(trigger.geom, 0x0002);
    dGeomSetCollideBits (trigger.geom, 0x0004);

    ctx.player_geom  = player.geom;
    ctx.trigger_geom = trigger.geom;

    InitWindow(1000, 700, "ODE Trigger – FINAL (R=reset)");
    SetTargetFPS(60);
    DisableCursor();

    camera.position = (Vector3){0, 10, 20};
    camera.target   = (Vector3){0, 5, 0};
    camera.up       = (Vector3){0, 1, 0};
    camera.fovy     = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Mesh mesh = GenMeshCube(1,1,1);
    Model model = LoadModelFromMesh(mesh);

    resetPlayer();

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_R)) resetPlayer();

        playerInTrigger = 0;
        dSpaceCollide(space, &ctx, &nearCallback);
        dWorldQuickStep(world, 1.0f/60.0f);
        dJointGroupEmpty(contact_group);

        if (playerInTrigger && !enteredPrinted) {
            printf(">>> TRIGGER ENTER\n");
            enteredPrinted = 1;
            exitedPrinted  = 0;
            trigger.color = GREEN;
        }
        else if (!playerInTrigger && enteredPrinted && !exitedPrinted) {
            printf(">>> TRIGGER EXIT\n");
            exitedPrinted = 1;
            trigger.color = BLUE;
        }

        updateCamera();

        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);

        DrawPlane((Vector3){0,0,0}, (Vector2){50,50}, LIGHTGRAY);

        // Player
        model.transform = odeToRLTransform(player.body);
        DrawModel(model, (Vector3){0,0,0}, player.size, player.color);
        DrawModelWires(model, (Vector3){0,0,0}, player.size, BLACK);

        // Trigger — VISIBLE WIREFRAME
        const dReal *tp = dBodyGetPosition(trigger.body);
        DrawCubeWires((Vector3){tp[0], tp[1], tp[2]}, triggerSize, triggerSize, triggerSize, trigger.color);

        EndMode3D();

        DrawFPS(10,10);
        DrawText("WASD+Mouse | R=reset", 10,35,20,DARKGRAY);
        DrawText(TextFormat("In Trigger: %s", playerInTrigger?"YES":"NO"),
                  10,60,20, playerInTrigger?GREEN:DARKGRAY);
        EndDrawing();
    }

    UnloadModel(model);
    UnloadMesh(mesh);
    dJointGroupDestroy(contact_group);
    dSpaceDestroy(space);
    dWorldDestroy(world);
    dCloseODE();
    CloseWindow();
    return 0;
}