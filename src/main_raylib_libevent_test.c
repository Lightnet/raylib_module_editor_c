// raylib 5.5
// flecs v4.1.1

// https://www.raylib.com/cheatsheet/cheatsheet.html

#include <stdio.h>
#include "ecs_components.h"
#include "module_libevent.h"
#include "module_dev.h"
#include "raygui.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MOUSE_SENSITIVITY 0.002f
#define MOVE_SPEED 5.0f
#define SPRINT_MULTIPLIER 2.0f
#define CAMERA_FOV 60.0f
#define CAMERA_MIN_DISTANCE 0.1f
#define MAX_PITCH 89.0f * DEG2RAD

// Define the name_t struct
typedef struct {
    float yaw;
    float pitch;
} camera_controller_t;
ECS_COMPONENT_DECLARE(camera_controller_t);

// draw raylib grid
void render_3d_grid(ecs_iter_t *it){
    DrawGrid(10, 1.0f);
}

// camera input
void camera_input_system(ecs_iter_t *it){
    main_context_t *main_context = ecs_field(it, main_context_t, 0);
    camera_controller_t *camera_controller = ecs_field(it, camera_controller_t, 1);

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        if (IsCursorHidden()) EnableCursor();
        else DisableCursor();
    }

    if(IsCursorHidden()){
        // printf("...\n");
        // Get mouse delta (captured mouse movement)
        Vector2 mouseDelta = GetMouseDelta();
        
        // Only update rotation when mouse is captured
        if (IsWindowFocused())
        {
            // Update yaw and pitch from mouse delta
            camera_controller->yaw += mouseDelta.x * MOUSE_SENSITIVITY;
            camera_controller->pitch -= mouseDelta.y * MOUSE_SENSITIVITY;

            // Clamp pitch to prevent flipping
            camera_controller->pitch = Clamp(camera_controller->pitch, -MAX_PITCH, MAX_PITCH);

            // printf("pitch:%f yaw:%f\n", camera_controller->pitch, camera_controller->yaw);
        }

        // Calculate forward direction using spherical coordinates
        Vector3 forward;
        forward.x = cosf(camera_controller->yaw) * cosf(camera_controller->pitch);
        forward.y = sinf(camera_controller->pitch);
        forward.z = sinf(camera_controller->yaw) * cosf(camera_controller->pitch);
        forward = Vector3Normalize(forward);

        // Update camera target
        main_context->camera.target = Vector3Add(main_context->camera.position, forward);

        // Movement input
        Vector3 velocity = { 0.0f, 0.0f, 0.0f };
        float speed = MOVE_SPEED;

        // Sprint
        if (IsKeyDown(KEY_LEFT_SHIFT))
            speed *= SPRINT_MULTIPLIER;

        // Get normalized forward direction (horizontal)
        Vector3 forwardFlat = forward;
        forwardFlat.y = 0.0f;
        forwardFlat = Vector3Normalize(forwardFlat);

        // Get right vector
        Vector3 right = Vector3CrossProduct(forwardFlat, main_context->camera.up);
        right = Vector3Normalize(right);

        // WASD movement
        if (IsKeyDown(KEY_W))
            velocity = Vector3Add(velocity, Vector3Scale(forwardFlat, speed));
        if (IsKeyDown(KEY_S))
            velocity = Vector3Subtract(velocity, Vector3Scale(forwardFlat, speed));
        if (IsKeyDown(KEY_A))
            velocity = Vector3Subtract(velocity, Vector3Scale(right, speed));
        if (IsKeyDown(KEY_D))
            velocity = Vector3Add(velocity, Vector3Scale(right, speed));

        // Vertical movement (spectator mode)
        if (IsKeyDown(KEY_SPACE))
            velocity.y += speed;
        if (IsKeyDown(KEY_LEFT_CONTROL))
            velocity.y -= speed;

        // Apply movement
        Vector3 movement = Vector3Scale(velocity, GetFrameTime());
        main_context->camera.position = Vector3Add(main_context->camera.position, movement);
        main_context->camera.target = Vector3Add(main_context->camera.target, movement);

        // Keep cursor centered
        SetMousePosition(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    }

    // Camera3D *camera = &sceneContext->camera;
    // if (IsCursorHidden()) UpdateCamera(&camera, CAMERA_FIRST_PERSON);
    // if (IsCursorHidden()) UpdateCamera(&main_context->camera, CAMERA_FIRST_PERSON);
    // // Toggle camera controls
    // if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    // {
    //     if (IsCursorHidden()) EnableCursor();
    //     else DisableCursor();
    // }
}

// draw 2d crosshair
void render_2d_draw_cross_point(ecs_iter_t *it){
    DrawCircleLines((int)(WINDOW_WIDTH/2), (int)(WINDOW_HEIGHT/2), 8, DARKBLUE);
}

// draw server and client network
void render_2d_libevent_system(ecs_iter_t *it){
    const libevent_context_t *app = ecs_singleton_get(it->world, libevent_context_t);

    if (app) {
        DrawText(app->status, 70, 22 * 1, 20, DARKGRAY);
    }
    
    if(GuiButton((Rectangle){0,22*1,64,20},"server")){
        printf("server\n");
        const libevent_server_t *libevent_server = ecs_singleton_get(it->world, libevent_server_t);
        if(!libevent_server){
            //server setup
            ecs_singleton_set(it->world, libevent_server_t,{
                .listener = NULL,
                .port = 8080,
                .is_init = false
            });
        }

    }
    if(GuiButton((Rectangle){0,22*2,64,20},"client")){
        printf("client\n");
        //server setup
        ecs_singleton_set(it->world, libevent_client_t,{
            .client_bev = NULL,
            .address = "127.0.0.1",
            .port = 8080,
            .is_init = false,
        });
    }

    if(GuiButton((Rectangle){0,22*3,64,20},"ping")){
        printf("ping\n");
        const libevent_client_t *libevent_client = ecs_singleton_get_mut(it->world, libevent_client_t);
        if(libevent_client){
            bufferevent_write(libevent_client->client_bev, "PING", 4);
        }
    }

    if(GuiButton((Rectangle){0,22*4,64,20},"get clients")){
        printf("get clients\n");
        const libevent_server_t *libevent_server2 = ecs_singleton_get(it->world, libevent_server_t);

        if(libevent_server2){
            int client_nums = 0;
            ecs_query_t *q = ecs_query(it->world, {
                .terms = {
                    {  .id = ecs_id(libevent_bev_t) }
                }
            });
            ecs_iter_t qit = ecs_query_iter(it->world, q);
            while (ecs_query_next(&qit)) {
                // libevent_bev_t *libevent_bev = ecs_field(&it, libevent_bev_t, 0);
                client_nums += qit.count;
                // for (int i = 0; i < it.count; i++) {
                    // ecs_entity_t client = it.entities[i];
                    // Do the thing
                    // if(bev == libevent_bev[i].bev){
                    //     ecs_delete(app->world, client);
                    //     printf("[server] remove client.\n");
                    // }
                // }
            }
            ecs_query_fini(q);

            printf("client(s) %d\n", client_nums);
            
        }
    }

}

// main
int main(void) {

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    InitWindow(800, 600, "Raylib LibEvent Flecs v4.1.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();

    ECS_COMPONENT_DEFINE(world, camera_controller_t);

    // Initialize components and phases
    module_init_raylib(world);
    module_init_dev(world);
    module_init_libevent(world);

    ECS_SYSTEM(world, render_3d_grid, RLRender3DPhase);

    // Input for camera
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "network_input_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) }, // Singleton
            { .id = ecs_id(camera_controller_t), .src.id = ecs_id(camera_controller_t) }   // Singleton
        },
        .callback = camera_input_system
    });

    // draw center crosshair
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_2d_draw_cross_point", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
        .callback = render_2d_draw_cross_point
    });

    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_2d_libevent_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
        .callback = render_2d_libevent_system
    });

    // setup Camera 3D
    Camera3D camera = {
        .position = (Vector3){10.0f, 10.0f, 10.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };
    ecs_singleton_set(world, main_context_t, {
        .camera = camera
    });

    ecs_singleton_set(world, camera_controller_t, {
        .yaw = 22.792f,
        .pitch = -0.592f,
    });

    // libevent context, required
    ecs_singleton_set(world, libevent_context_t,{
        .world = world,
        .pings_sent = 0,
        .pongs_received = 0,
        .status = "None"
    });

    //Loop Logic and render
    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    // UnloadModel(cube);
    WSACleanup();
    ecs_fini(world);
    CloseWindow();
    return 0;
}
