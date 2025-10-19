#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NODRAWTEXT        // DrawText() and DT_*
    #define NOGDI             // All GDI defines and routines
    // #define NOKERNEL          // All KERNEL defines and routines
    #define NOUSER            // All USER defines and routines
    #define NOSOUND           // Sound driver routines
    #include <winsock2.h>
    #include <ws2tcpip.h>
    // #include <windows.h>
#else
    #error This code is designed for Windows only
#endif

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#include <stdio.h>
#include <string.h>

#define SERVER_PORT 8080
#define SERVER_ADDRESS "127.0.0.1"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// Global variables
struct event_base *server_base = NULL, *client_base = NULL;
struct bufferevent *client_bev = NULL;
struct evconnlistener *listener = NULL;
bool server_running = false, client_connected = false;
bool ping_response_received = false;
char ping_result[256] = "No ping test yet";
HANDLE server_thread_id, client_thread_id;

// Server: Handle client read
void server_read_cb(struct bufferevent *bev, void *ctx) {
    char buf[256];
    size_t n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    if (strcmp(buf, "PING") == 0) {
        bufferevent_write(bev, "PONG", 4);
    }
}

// Server: Accept new connections
void server_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *ptr) {
    struct event_base *base = ptr;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, server_read_cb, NULL, NULL, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

// Client: Handle server response
void client_read_cb(struct bufferevent *bev, void *ctx) {
    char buf[256];
    size_t n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    snprintf(ping_result, sizeof(ping_result), "Received: %s", buf);
    ping_response_received = true;
}

// Server: Thread function
DWORD WINAPI server_thread(LPVOID arg) {
    if (server_base) {
        event_base_dispatch(server_base);
    }
    return 0;
}

// Client: Thread function
DWORD WINAPI client_thread(LPVOID arg) {
    if (client_base) {
        event_base_dispatch(client_base);
    }
    return 0;
}

// Server: Initialize
void server_init() {
    if (server_running) return;
    server_base = event_base_new();
    if (!server_base) {
        strcpy(ping_result, "Server init failed");
        return;
    }
    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);
    listener = evconnlistener_new_bind(server_base, server_accept_cb, server_base,
                                      LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                      (struct sockaddr *)&sin, sizeof(sin));
    if (!listener) {
        strcpy(ping_result, "Listener creation failed");
        event_base_free(server_base);
        server_base = NULL;
        return;
    }
    server_running = true;
    server_thread_id = CreateThread(NULL, 0, server_thread, NULL, 0, NULL);
    if (!server_thread_id) {
        strcpy(ping_result, "Server thread creation failed");
        evconnlistener_free(listener);
        event_base_free(server_base);
        listener = NULL;
        server_base = NULL;
        server_running = false;
        return;
    }
    strcpy(ping_result, "Server initialized");
}

// Client: Initialize
void client_init() {
    if (client_connected) return;
    client_base = event_base_new();
    if (!client_base) {
        strcpy(ping_result, "Client init failed");
        return;
    }
    client_bev = bufferevent_socket_new(client_base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!client_bev) {
        strcpy(ping_result, "Client bev creation failed");
        event_base_free(client_base);
        client_base = NULL;
        return;
    }
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr);
    bufferevent_setcb(client_bev, client_read_cb, NULL, NULL, NULL);
    bufferevent_enable(client_bev, EV_READ | EV_WRITE);
    if (bufferevent_socket_connect(client_bev, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        strcpy(ping_result, "Client connect failed");
        bufferevent_free(client_bev);
        event_base_free(client_base);
        client_bev = NULL;
        client_base = NULL;
        return;
    }
    client_connected = true;
    client_thread_id = CreateThread(NULL, 0, client_thread, NULL, 0, NULL);
    if (!client_thread_id) {
        strcpy(ping_result, "Client thread creation failed");
        bufferevent_free(client_bev);
        event_base_free(client_base);
        client_bev = NULL;
        client_base = NULL;
        client_connected = false;
        return;
    }
    strcpy(ping_result, "Client initialized");
}

// Main function
int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Initialize raylib
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Server-Client Ping Test");
    SetTargetFPS(60);

    // Main loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw buttons and status
        if (GuiButton((Rectangle){100, 100, 200, 50}, "Init Server")) {
            server_init();
        }
        if (GuiButton((Rectangle){100, 160, 200, 50}, "Init Client")) {
            client_init();
        }
        if (GuiButton((Rectangle){100, 220, 200, 50}, "Send Ping")) {
            if (server_running && client_connected && client_bev) {
                ping_response_received = false;
                strcpy(ping_result, "Waiting for response...");
                bufferevent_write(client_bev, "PING", 4);
            } else {
                strcpy(ping_result, "Server or client not initialized");
            }
        }
        DrawText(ping_result, 100, 300, 20, BLACK);
        DrawText(server_running ? "Server: Running" : "Server: Not Running", 100, 350, 20, server_running ? GREEN : RED);
        DrawText(client_connected ? "Client: Connected" : "Client: Not Connected", 100, 380, 20, client_connected ? GREEN : RED);

        EndDrawing();
    }

    // Cleanup
    if (client_bev) bufferevent_free(client_bev);
    if (client_base) event_base_free(client_base);
    if (listener) evconnlistener_free(listener);
    if (server_base) event_base_free(server_base);
    if (server_thread_id) CloseHandle(server_thread_id);
    if (client_thread_id) CloseHandle(client_thread_id);
    WSACleanup();
    CloseWindow();

    return 0;
}