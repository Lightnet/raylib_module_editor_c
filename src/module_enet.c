// module_enet.c
/*
- system set up for server or client
- system loop with condtions
- system pass event for flecs
- system filter 

*/
#include <stdio.h>

#include "ecs_components.h" // phase
#include "module_enet.h"
// #include "raylib.h"
#include "raygui.h"

// Declare and define component in the source file
ECS_COMPONENT_DECLARE(enet_packet_t);
ECS_COMPONENT_DECLARE(NetworkConfig);
ECS_COMPONENT_DECLARE(NetworkState);

// Declare event entity (defined in setup_components_enet)
// ecs_entity_t event_receive_packed = 0;
ecs_entity_t event_receive_packed;
ENetHost *g_host = NULL;  // Global for simplicity (or store in singleton)


// Render HUD system
void render2d_hud_enet_system(ecs_iter_t *it) {
    const NetworkConfig *config = ecs_singleton_get(it->world, NetworkConfig);
    const NetworkState *state = ecs_singleton_get(it->world, NetworkState);

    if (!config) {
        printf("No config!\n");
        return;
    }

    int y_offset = 2;
    const int font_size = 20;
    const int spacing = 25;

    DrawText("Network Status:", 2, y_offset, font_size, DARKGRAY);
    y_offset += spacing;

    if (!config->isNetwork) {
        DrawText("Network: Disabled", 2, y_offset, font_size, DARKGRAY);
    } else if (state->serverStarted) {
        char status[64];
        snprintf(status, sizeof(status), "Server: Running on port %d", config->port);
        DrawText(status, 2, y_offset, font_size, DARKGRAY);
    } else if (config->isServer) {
        DrawText("Server: Not started", 2, y_offset, font_size, DARKGRAY);
    } else if (state->clientConnected && state->isConnected) {
        char status[64];
        snprintf(status, sizeof(status), "Client: Connected to %s:%d", config->address, config->port);
        DrawText(status, 2, y_offset, font_size, DARKGRAY);
    } else if (state->clientConnected) {
        char status[64];
        snprintf(status, sizeof(status), "Client: Connecting to %s:%d", config->address, config->port);
        DrawText(status, 2, y_offset, font_size, DARKGRAY);
    } else {
        DrawText("Client: Disconnected", 2, y_offset, font_size, DARKGRAY);
    }
}

// enet packet for string data
void on_receive_packed(ecs_iter_t *it) {
    printf("event on_receive_packed\n");
    enet_packet_t *p = it->param;
    if (p && p->data) {
        char *str = (char*)p->data;
        printf("Received string: %s\n", str);
        free(str); // Free the dynamically allocated string
    } else {
        printf("No valid string data received\n");
    }
}

void test_input_enet_system(ecs_iter_t *it){
    if (IsKeyDown(KEY_R)) {
        char *str = strdup("hello"); // Dynamically allocate string
        enet_packet_t packet = { .data = str };
        ecs_emit(it->world, &(ecs_event_desc_t) {
            .event = ecs_id(enet_packet_t),
            .entity = event_receive_packed,
            .param = &packet
        });
        // Note: str must be freed later, e.g., in the observer or a cleanup system
    }
}

// Network initialization system
void network_init_system(ecs_iter_t *it) {
    NetworkConfig *config = ecs_singleton_get_mut(it->world, NetworkConfig);
    NetworkState *state = ecs_singleton_get_mut(it->world, NetworkState);

    if (!config || !state) {
        printf("Missing config or state\n");
        return;
    }

    if (config->isNetwork && config->isServer && !state->serverStarted) {
        if (enet_initialize() != 0) {
            printf("ENet init failed\n");
            return;
        }
        ENetAddress address = {0};
        address.host = ENET_HOST_ANY;
        address.port = config->port; // Use config port
        state->host = enet_host_create(&address, config->maxPeers, 1, 0, 0);
        if (!state->host) {
            printf("Server host creation failed\n");
            enet_deinitialize();
            return;
        }
        state->serverStarted = true;
        state->isServer = true;
        printf("Server started on port %d\n", config->port);
        ecs_singleton_modified(it->world, NetworkState);
    } else if (config->isNetwork && !config->isServer && !state->clientConnected) {
        if (enet_initialize() != 0) {
            printf("ENet init failed\n");
            return;
        }
        state->host = enet_host_create(NULL, 1, 1, 0, 0);
        if (!state->host) {
            printf("Client host creation failed\n");
            enet_deinitialize();
            return;
        }
        ENetAddress address = {0};
        if (enet_address_set_host(&address, config->address) != 0) {
            printf("Failed to set host address: %s\n", config->address);
            enet_host_destroy(state->host);
            state->host = NULL;
            enet_deinitialize();
            return;
        }
        address.port = config->port;
        state->peer = enet_host_connect(state->host, &address, 1, 0);
        if (!state->peer) {
            printf("Client connect failed\n");
            enet_host_destroy(state->host);
            state->host = NULL;
            enet_deinitialize();
            return;
        }
        state->clientConnected = true;
        state->isServer = false;
        printf("Client attempting connect to %s:%d\n", config->address, config->port);
        ecs_singleton_modified(it->world, NetworkState);
    }
}

// Network service system
void network_service_system(ecs_iter_t *it) {
    NetworkState *state = ecs_singleton_get_mut(it->world, NetworkState);
    if (!state || !state->host) {
        return;
    }

    ENetEvent event;
    while (enet_host_service(state->host, &event, 0) > 0) {
        printf("Network event: %d\n", event.type);
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connected! Peer ID: %u\n", event.peer->incomingPeerID);
                state->isConnected = true;
                if (!state->isServer) {
                    state->clientConnected = true;
                }
                ecs_singleton_modified(it->world, NetworkState);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                printf("Disconnected\n");
                state->isConnected = false;
                if (!state->isServer) {
                    state->clientConnected = false;
                }
                ecs_singleton_modified(it->world, NetworkState);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                printf("Received packet from peer %u, channel %u, size %zu\n",
                       event.peer->incomingPeerID, event.channelID, event.packet->dataLength);
                if (event.packet->dataLength > 0) {
                    // Copy data to avoid use-after-free
                    char *data_copy = strdup((char *)event.packet->data);
                    enet_packet_t packet = { .data = data_copy };
                    ecs_emit(it->world, &(ecs_event_desc_t) {
                        .event = ecs_id(enet_packet_t),
                        .entity = event_receive_packed,
                        .param = &packet
                    });
                }
                enet_packet_destroy(event.packet);
                break;
            default:
                break;
        }
    }

    // Send periodic packets (like standalone code)
    if (state->isServer && state->serverStarted) {
        for (size_t i = 0; i < state->host->peerCount; i++) {
            ENetPeer *peer = &state->host->peers[i];
            if (peer->state == ENET_PEER_STATE_CONNECTED) {
                ENetPacket *packet = enet_packet_create("HELLO WORLD", strlen("HELLO WORLD") + 1, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(peer, 0, packet);
            }
        }
        enet_host_flush(state->host);
    } else if (state->isConnected && !state->isServer && state->peer) {
        if (state->peer->state == ENET_PEER_STATE_CONNECTED) {
            ENetPacket *packet = enet_packet_create("HELLO WORLD", strlen("HELLO WORLD") + 1, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(state->peer, 0, packet);
            enet_host_flush(state->host);
        }
    }
}


// Network input system
void network_input_system(ecs_iter_t *it) {
    NetworkConfig *config = ecs_singleton_get_mut(it->world, NetworkConfig);
    NetworkState *state = ecs_singleton_get_mut(it->world, NetworkState);

    if (!config) {
        ecs_singleton_set(it->world, NetworkConfig, {
            .isNetwork = false,
            .isServer = false,
            .port = 1234,
            .maxPeers = 32,
            .address = "127.0.0.1"
        });
        config = ecs_singleton_get_mut(it->world, NetworkConfig);
    }

    if (IsKeyPressed(KEY_S) && !state->serverStarted && !state->clientConnected) {
        ecs_singleton_set(it->world, NetworkConfig, {
            .isNetwork = true,
            .isServer = true,
            .port = 1234,
            .maxPeers = 32,
            .address = "127.0.0.1"
        });
        printf("Triggering server start...\n");
    }
    if (IsKeyPressed(KEY_C) && !state->serverStarted && !state->clientConnected) {
        ecs_singleton_set(it->world, NetworkConfig, {
            .isNetwork = true,
            .isServer = false,
            .port = 1234,
            .maxPeers = 32,
            .address = "127.0.0.1"
        });
        printf("Triggering client connect...\n");
    }
    if (IsKeyPressed(KEY_T) && state->clientConnected && state->peer) {
        ENetPacket *packet = enet_packet_create("test message", strlen("test message") + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(state->peer, 0, packet);
        enet_host_flush(state->host);
        printf("Sent test packet\n");
    }
    if (IsKeyPressed(KEY_Y) && state->clientConnected && state->peer) {
        printf("Client state: %d\n", state->peer->state);
    }

    if (config) {
        ecs_singleton_modified(it->world, NetworkConfig);
    }
    if (state) {
        ecs_singleton_modified(it->world, NetworkState);
    }
}


void setup_systems_enet(ecs_world_t *world){

    // Input
    ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "test_input_enet_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
      //.query.terms = {
        //   { .id = ecs_id(Transform3D), .src.id = EcsSelf },
        //   { .id = ecs_pair(EcsChildOf, EcsWildcard), .oper = EcsNot }
      //},
      .callback = test_input_enet_system
    });

    // ONLY 2D
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "render2d_hud_debug_system", .add = ecs_ids(ecs_dependson(RLRender2D1Phase)) }),
        // .query.terms = {
        //     { .id = ecs_id(NetworkConfig), .src.id = ecs_id(NetworkConfig) }, // Singleton
        //     { .id = ecs_id(NetworkState), .src.id = ecs_id(NetworkState) }   // Singleton
        // },
      .callback = render2d_hud_enet_system
    });

    // Create an entity observer
    ecs_observer(world, {
        // Not interested in any specific component
        .query.terms = {{ EcsAny, .src.id = event_receive_packed }},
        .events = { ecs_id(enet_packet_t) },
        .callback = on_receive_packed
    });


    // New systems
    // Init on add of NetworkConfig + NetworkState
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "network_init_system", .add = ecs_ids(ecs_dependson(PreLogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(NetworkConfig), .src.id = ecs_id(NetworkConfig) }, // Singleton
            { .id = ecs_id(NetworkState), .src.id = ecs_id(NetworkState) }   // Singleton
        },
        .callback = network_init_system
    });

    // Service in logic update
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "network_service_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        // .query.terms = {{ .id = ecs_id(NetworkState), .src.id = ecs_id(NetworkState) }}, // Singleton
        .callback = network_service_system
    });

    // Input for triggers
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "network_input_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(NetworkConfig), .src.id = ecs_id(NetworkConfig) }, // Singleton
            { .id = ecs_id(NetworkState), .src.id = ecs_id(NetworkState) }   // Singleton
        },
        .callback = network_input_system
    });
}

// component definitions
void setup_components_enet(ecs_world_t *world){
    // Define the enet_packet_t component (called only once)
    ECS_COMPONENT_DEFINE(world, enet_packet_t); 
    ECS_COMPONENT_DEFINE(world, NetworkConfig);
    ECS_COMPONENT_DEFINE(world, NetworkState);

    // Define the event entity
    event_receive_packed = ecs_entity(world, { .name = "receive_packed" });
}


void module_init_enet(ecs_world_t *world){
    // Define components
    setup_components_enet(world);
    //set up systems
    setup_systems_enet(world);
}