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

// server network component
typedef struct {
    struct evconnlistener *listener;
    // int port;
    uint16_t port; // Changed to uint16_t
    bool is_init; //prevent overlap set up
} libevent_server_t;
extern ECS_COMPONENT_DECLARE(libevent_server_t);

// client network component
typedef struct {
    struct bufferevent *client_bev;
    char address[INET_ADDRSTRLEN]; // Use INET_ADDRSTRLEN for IPv4 addresses
    uint16_t port; // Changed to uint16_t
    bool is_init; // Prevent overlap setup
} libevent_client_t;
extern ECS_COMPONENT_DECLARE(libevent_client_t);

// server client handle
typedef struct {
    struct bufferevent *bev;
} libevent_bev_t;
extern ECS_COMPONENT_DECLARE(libevent_bev_t);// server client

// network loop
typedef struct {
    struct event_base *ev_base;
} libevent_base_t;
extern ECS_COMPONENT_DECLARE(libevent_base_t);

//libevent context
typedef struct {
    ecs_world_t *world;
    int pings_sent;
    int pongs_received;
    char status[256];
} libevent_context_t;
extern ECS_COMPONENT_DECLARE(libevent_context_t);

// libevent packet component
typedef struct {
    void *data;       // Pointer to the message data (header + payload)
    size_t length;    // Total length of the data
    struct bufferevent *bev; // Source bufferevent
} libevent_packet_t;
extern ECS_COMPONENT_DECLARE(libevent_packet_t);

// Declare event entity as extern for global access
extern ecs_entity_t libevent_receive_packed;


struct GameMessage {
    uint16_t type;    // Message type (e.g., 1 = Position, 2 = Health, 3 = Chat)
    uint16_t length;  // Length of payload in bytes
    uint8_t payload[]; // Variable-length payload (flexible array member)
};


void module_init_libevent(ecs_world_t *world); 