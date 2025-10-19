#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NODRAWTEXT
    #define NOGDI
    #define NOUSER
    #define NOSOUND
    #include <winsock2.h>
    #include <ws2tcpip.h>
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
#define MAX_CLIENTS 10

// Global variables
struct event_base *ev_base = NULL;
struct bufferevent *client_bev = NULL;
struct evconnlistener *listener = NULL;
bool is_server = false, is_client = false;
char ping_result[256] = "No ping test yet";
struct bufferevent *connected_clients[MAX_CLIENTS] = {0};
int num_clients = 0;

// Server: Handle client read
void server_read_cb(struct bufferevent *bev, void *ctx) {
    char buf[256];
    size_t n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    if (strcmp(buf, "PING") == 0) {
        bufferevent_write(bev, "PONG", 4);
        snprintf(ping_result, sizeof(ping_result), "Server: Received PING, sent PONG");
    } else if (strcmp(buf, "PONG") == 0) {
        snprintf(ping_result, sizeof(ping_result), "Server: Received PONG");
    } else {
        snprintf(ping_result, sizeof(ping_result), "Server: Unknown message: %s", buf);
    }
}

// Server: Handle client errors or disconnection
void server_error_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        for (int i = 0; i < num_clients; i++) {
            if (connected_clients[i] == bev) {
                connected_clients[i] = NULL;
                // Shift remaining clients to fill the gap
                for (int j = i; j < num_clients - 1; j++) {
                    connected_clients[j] = connected_clients[j + 1];
                }
                connected_clients[num_clients - 1] = NULL;
                num_clients--;
                snprintf(ping_result, sizeof(ping_result), "Server: Client %d disconnected", i + 1);
                break;
            }
        }
        bufferevent_free(bev);
    }
}

// Server: Accept new connections
void server_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *ptr) {
    struct event_base *base = ptr;
    if (num_clients >= MAX_CLIENTS) {
        closesocket(fd);
        snprintf(ping_result, sizeof(ping_result), "Server: Max clients reached");
        return;
    }
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        closesocket(fd);
        snprintf(ping_result, sizeof(ping_result), "Server: Failed to create bufferevent");
        return;
    }
    bufferevent_setcb(bev, server_read_cb, NULL, server_error_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    connected_clients[num_clients++] = bev;
    snprintf(ping_result, sizeof(ping_result), "Server: Client %d connected", num_clients);
}

// Client: Handle server response
void client_read_cb(struct bufferevent *bev, void *ctx) {
    char buf[256];
    size_t n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    if (strcmp(buf, "PING") == 0) {
        bufferevent_write(bev, "PONG", 4);
        snprintf(ping_result, sizeof(ping_result), "Client: Received PING, sent PONG");
    } else if (strcmp(buf, "PONG") == 0) {
        snprintf(ping_result, sizeof(ping_result), "Client: Received PONG");
    } else {
        snprintf(ping_result, sizeof(ping_result), "Client: Unknown message: %s", buf);
    }
}

// Client: Error callback
void client_error_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_ERROR) {
        snprintf(ping_result, sizeof(ping_result), "Client: Error %d", EVUTIL_SOCKET_ERROR());
    } else if (events & BEV_EVENT_EOF) {
        snprintf(ping_result, sizeof(ping_result), "Client: Connection closed");
        bufferevent_free(bev);
        client_bev = NULL;
        is_client = false;
    }
}

// Server: Initialize
void server_init() {
    if (is_server) return;
    if (!ev_base) {
        ev_base = event_base_new();
        if (!ev_base) {
            strcpy(ping_result, "Event base creation failed");
            return;
        }
    }
    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);
    listener = evconnlistener_new_bind(ev_base, server_accept_cb, ev_base,
                                      LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                      (struct sockaddr *)&sin, sizeof(sin));
    if (!listener) {
        strcpy(ping_result, "Listener creation failed");
        if (!is_client) {
            event_base_free(ev_base);
            ev_base = NULL;
        }
        return;
    }
    is_server = true;
    strcpy(ping_result, "Server initialized");
}

// Client: Initialize
void client_init() {
    if (is_client) return;
    if (!ev_base) {
        ev_base = event_base_new();
        if (!ev_base) {
            strcpy(ping_result, "Event base creation failed");
            return;
        }
    }
    client_bev = bufferevent_socket_new(ev_base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!client_bev) {
        strcpy(ping_result, "Client bev creation failed");
        if (!is_server) {
            event_base_free(ev_base);
            ev_base = NULL;
        }
        return;
    }
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr);
    bufferevent_setcb(client_bev, client_read_cb, NULL, client_error_cb, NULL);
    bufferevent_enable(client_bev, EV_READ | EV_WRITE);
    if (bufferevent_socket_connect(client_bev, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        strcpy(ping_result, "Client connect failed");
        bufferevent_free(client_bev);
        client_bev = NULL;
        if (!is_server) {
            event_base_free(ev_base);
            ev_base = NULL;
        }
        return;
    }
    is_client = true;
    strcpy(ping_result, "Client initialized");
}

// Server: Send ping to all connected clients
void server_send_ping() {
    if (!is_server || num_clients == 0) {
        strcpy(ping_result, "No clients connected or server not initialized");
        return;
    }
    strcpy(ping_result, "Server: Waiting for PONG...");
    for (int i = 0; i < num_clients; i++) {
        if (connected_clients[i]) {
            bufferevent_write(connected_clients[i], "PING", 4);
        }
    }
}

// Client: Send ping to server
void client_send_ping() {
    if (!is_client || !client_bev) {
        strcpy(ping_result, "Client not initialized");
        return;
    }
    strcpy(ping_result, "Client: Waiting for PONG...");
    bufferevent_write(client_bev, "PING", 4);
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
        // Process libevent events (non-blocking, multiple calls for reliability)
        if (ev_base) {
            event_base_loop(ev_base, EVLOOP_NONBLOCK);
            // event_base_loop(ev_base, EVLOOP_NONBLOCK);
            // event_base_loop(ev_base, EVLOOP_NONBLOCK);
        }

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
            if (is_server) {
                server_send_ping();
            } else if (is_client) {
                client_send_ping();
            } else {
                strcpy(ping_result, "Server or client not initialized");
            }
        }
        DrawText(ping_result, 100, 300, 20, BLACK);
        DrawText(is_server ? "Server: Running" : "Server: Not Running", 100, 350, 20, is_server ? GREEN : RED);
        DrawText(is_client ? "Client: Connected" : "Client: Not Connected", 100, 380, 20, is_client ? GREEN : RED);
        DrawText(TextFormat("Connected Clients: %d", num_clients), 100, 410, 20, BLACK);

        EndDrawing();
    }

    // Cleanup
    if (client_bev) bufferevent_free(client_bev);
    if (listener) evconnlistener_free(listener);
    for (int i = 0; i < num_clients; i++) {
        if (connected_clients[i]) bufferevent_free(connected_clients[i]);
    }
    if (ev_base) event_base_free(ev_base);
    WSACleanup();
    CloseWindow();

    return 0;
}