// https://github.com/libevent/libevent.git
#pragma once
// 

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

#include "flecs.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <stdbool.h>

// TransformGUI component
typedef struct {
    struct evconnlistener *listener;
    // int port;
    uint16_t port; // Changed to uint16_t
    bool is_init; //prevent overlap set up
} libevent_server_t;
extern ECS_COMPONENT_DECLARE(libevent_server_t);

// typedef struct {
//     struct bufferevent *client_bev;
//     char address[128];
//     int port;
//     bool is_init; //prevent overlap set up
// } libevent_client_t;
// extern ECS_COMPONENT_DECLARE(libevent_client_t);

typedef struct {
    struct bufferevent *client_bev;
    char address[INET_ADDRSTRLEN]; // Use INET_ADDRSTRLEN for IPv4 addresses
    uint16_t port; // Changed to uint16_t
    bool is_init; // Prevent overlap setup
} libevent_client_t;
extern ECS_COMPONENT_DECLARE(libevent_client_t);


typedef struct {
    struct event_base *ev_base;
} libevent_base_t;
extern ECS_COMPONENT_DECLARE(libevent_base_t);

typedef struct {
    ecs_world_t *world;
    int pings_sent;
    int pongs_received;
    char status[256];
} libevent_context_t;
extern ECS_COMPONENT_DECLARE(libevent_context_t);

void module_init_libevent(ecs_world_t *world); 