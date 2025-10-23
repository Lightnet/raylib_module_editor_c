// module_libevent.c
// note that stacking on_receive_libevent_packed filter will crashed. might be misconfig.
#include "ecs_components.h"
#include "module_libevent.h"
#include <string.h>
#include <stdlib.h>

// Define global constants and storage for connected clients
int MAX_CLIENTS = 10;
// static struct bufferevent *connected_clients[MAX_CLIENTS] = {0};
// static int num_clients = 0;

// #define SERVER_PORT 8080
// #define SERVER_ADDRESS "127.0.0.1"

// Declare and define component in the source file
ECS_COMPONENT_DECLARE(libevent_base_t);
ECS_COMPONENT_DECLARE(libevent_server_t);
ECS_COMPONENT_DECLARE(libevent_client_t);
ECS_COMPONENT_DECLARE(libevent_context_t);
ECS_COMPONENT_DECLARE(libevent_bev_t);
ECS_COMPONENT_DECLARE(libevent_packet_t);

// event dispatch for network get data...
ecs_entity_t libevent_receive_packed;

//===============================================
// CALLBACKS
//===============================================
// Server: Handle client read
void server_read_cb(struct bufferevent *bev, void *ctx) {
    libevent_context_t *app = (libevent_context_t *)ctx;
    char buf[256];
    size_t n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    if (strcmp(buf, "PING") == 0) {
        bufferevent_write(bev, "PONG", 4);
        snprintf(app->status, sizeof(app->status), "Server: Received PING, sent PONG");
        printf("[server] ping\n");
    } else if (strcmp(buf, "PONG") == 0) {
        app->pongs_received++;
        snprintf(app->status, sizeof(app->status), "Server: Received PONG (%d)", app->pongs_received);
        printf("[server] pong\n");
    } else {
        printf("[server] unknown\n");
        snprintf(app->status, sizeof(app->status), "Server: Unknown message: %s", buf);
    }
    //ecs_singleton_modified(app->world, libevent_context_t);
    // Emit the received data as an ECS event
    if (app->world) {
        // Create a libevent_packet_t instance
        libevent_packet_t packet = {
            .data = strdup(buf), // Dynamically allocate a copy of buf
            .bev = bev
        };
        if (packet.data) { // Check if strdup succeeded
            ecs_emit(app->world, &(ecs_event_desc_t) {
                .event = ecs_id(libevent_packet_t),
                .entity = libevent_receive_packed,
                .param = &packet
            });
        } else {
            printf("[server] Failed to allocate memory for packet data\n");
        }
    }
}

/*
void server_read_cb(struct bufferevent *bev, void *ctx) {
    libevent_context_t *app = (libevent_context_t *)ctx;
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input); // Get available data size

    if (len < sizeof(uint16_t) * 2) {
        // Not enough data for header (type + length)
        return;
    }

    // Peek at the header to get the message length
    struct GameMessage header;
    if (evbuffer_copyout(input, &header, sizeof(header)) != sizeof(header)) {
        printf("[server] Failed to read header\n");
        return;
    }
    header.type = ntohs(header.type); // Convert to host byte order
    header.length = ntohs(header.length);

    // Ensure enough data is available for the full message
    if (len < sizeof(header) + header.length) {
        return; // Wait for more data
    }

    // Allocate buffer for the full message (header + payload)
    char *buf = malloc(sizeof(header) + header.length);
    if (!buf) {
        printf("[server] Failed to allocate memory\n");
        snprintf(app->status, sizeof(app->status), "Server: Memory allocation failed");
        ecs_singleton_modified(app->world, libevent_context_t);
        return;
    }

    // Copy the full message
    size_t n = evbuffer_copyout(input, buf, sizeof(header) + header.length);
    if (n != sizeof(header) + header.length) {
        free(buf);
        printf("[server] Failed to copy full message\n");
        return;
    }

    // Drain the processed data
    evbuffer_drain(input, n);

    // Process legacy PING/PONG for compatibility
    if (header.type == 0 && header.length == 4 && strncmp(buf + sizeof(header), "PING", 4) == 0) {
        bufferevent_write(bev, "PONG", 4);
        snprintf(app->status, sizeof(app->status), "Server: Received PING, sent PONG");
        printf("[server] ping\n");
    } else if (header.type == 0 && header.length == 4 && strncmp(buf + sizeof(header), "PONG", 4) == 0) {
        app->pongs_received++;
        snprintf(app->status, sizeof(app->status), "Server: Received PONG (%d)", app->pongs_received);
        printf("[server] pong\n");
    } else {
        // Process game object data based on type
        switch (header.type) {
            case 1: // Position update
                if (header.length == sizeof(float) * 3) {
                    float *pos = (float *)(buf + sizeof(header));
                    printf("[server] Position update: x=%.2f, y=%.2f, z=%.2f\n", pos[0], pos[1], pos[2]);
                    snprintf(app->status, sizeof(app->status), "Server: Received position update");
                }
                break;
            case 2: // Health update
                if (header.length == sizeof(int32_t)) {
                    int32_t *health = (int32_t *)(buf + sizeof(header));
                    printf("[server] Health update: %d\n", ntohl(*health));
                    snprintf(app->status, sizeof(app->status), "Server: Received health update");
                }
                break;
            default:
                printf("[server] Unknown message type: %d\n", header.type);
                snprintf(app->status, sizeof(app->status), "Server: Unknown message type: %d", header.type);
        }
    }

    // Emit ECS event
    if (app->world) {
        libevent_packet_t packet = {
            .data = buf, // Pass ownership to packet
            .length = n,
            .bev = bev
        };
        ecs_emit(app->world, &(ecs_event_desc_t) {
            .event = ecs_id(libevent_packet_t),
            .entity = libevent_receive_packed,
            .param = &packet
        });
    } else {
        free(buf);
    }

    ecs_singleton_modified(app->world, libevent_context_t);
}
*/

// Server: Handle client errors or disconnection
void server_error_cb(struct bufferevent *bev, short events, void *ctx) {
    libevent_context_t *app = (libevent_context_t *)ctx;
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        // for (int i = 0; i < num_clients; i++) {
        //     if (connected_clients[i] == bev) {
        //         connected_clients[i] = NULL;
        //         for (int j = i; j < num_clients - 1; j++) {
        //             connected_clients[j] = connected_clients[j + 1];
        //         }
        //         connected_clients[num_clients - 1] = NULL;
        //         num_clients--;
        //         snprintf(app->status, sizeof(app->status), "Server: Client %d disconnected", i + 1);
        //         break;
        //     }
        // }
        // handle remove client
        if(app->world){
            ecs_query_t *q = ecs_query(app->world, {
                .terms = {
                    {  .id = ecs_id(libevent_bev_t) }
                }
            });
            ecs_iter_t it = ecs_query_iter(app->world, q);
            while (ecs_query_next(&it)) {
                libevent_bev_t *libevent_bev = ecs_field(&it, libevent_bev_t, 0);
                for (int i = 0; i < it.count; i++) {
                    ecs_entity_t entity_client = it.entities[i];
                    // Do the thing
                    if(bev == libevent_bev[i].bev){
                        ecs_delete(app->world, entity_client);
                        printf("[server] remove client.\n");
                    }

                }
            }
            ecs_query_fini(q);
        }
        bufferevent_free(bev);
        // ecs_singleton_modified(app->world, libevent_context_t);
    }
}

// Server: Accept new connections
void server_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *ctx) {
    libevent_context_t *app = (libevent_context_t *)ctx;
    struct event_base *base = evconnlistener_get_base(listener);
    //TODO: need create query here for count.


    // if (num_clients >= MAX_CLIENTS) {
    //     closesocket(fd);
    //     snprintf(app->status, sizeof(app->status), "Server: Max clients reached");
    //     ecs_singleton_modified(app->world, libevent_context_t);
    //     return;
    // }
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        closesocket(fd);
        snprintf(app->status, sizeof(app->status), "Server: Failed to create bufferevent");
        ecs_singleton_modified(app->world, libevent_context_t);
        return;
    }
    bufferevent_setcb(bev, server_read_cb, NULL, server_error_cb, app);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    // connected_clients[num_clients++] = bev;

    if(app->world){
        ecs_entity_t e = ecs_new(app->world);
        ecs_set(app->world, e, libevent_bev_t, {
            .bev=bev
        });
    }

    // snprintf(app->status, sizeof(app->status), "Server: Client %d connected", num_clients);
    // ecs_singleton_modified(app->world, libevent_context_t);
}

// Client: Handle server response
void client_read_cb(struct bufferevent *bev, void *ctx) {
    libevent_context_t *app = (libevent_context_t *)ctx;
    char buf[256];
    size_t n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    if (strcmp(buf, "PING") == 0) {
        bufferevent_write(bev, "PONG", 4);
        snprintf(app->status, sizeof(app->status), "Client: Received PING, sent PONG");
        printf("[client] ping\n");
    } else if (strcmp(buf, "PONG") == 0) {
        app->pongs_received++;
        snprintf(app->status, sizeof(app->status), "Client: Received PONG (%d)", app->pongs_received);
        printf("[client] pong\n");
    } else {
        snprintf(app->status, sizeof(app->status), "Client: Unknown message: %s", buf);
        printf("[client] unknown\n");
    }
    ecs_singleton_modified(app->world, libevent_context_t);
}

//===============================================
// SYSTEMS
//===============================================
// Event loop system
void event_base_system(ecs_iter_t *it) {
    libevent_base_t *libevent_base = ecs_field(it, libevent_base_t, 0);
    if (!libevent_base || !libevent_base->ev_base) return;
    // printf("libevent_base_t\n");

    // Process libevent events
    event_base_loop(libevent_base->ev_base, EVLOOP_NONBLOCK);
}

// Server setup on add
void on_add_setup_server(ecs_iter_t *it) {
    libevent_server_t *libevent_server = ecs_field(it, libevent_server_t, 0);
    libevent_context_t *app = ecs_singleton_get_mut(it->world, libevent_context_t);
    if (!app) {
        ecs_singleton_set(it->world, libevent_context_t, {
            .world = it->world,
            .pings_sent = 0,
            .pongs_received = 0,
            .status = "Server: Initializing..."
        });
        app = ecs_singleton_get_mut(it->world, libevent_context_t);
    }

    if (libevent_server->is_init) {
        snprintf(app->status, sizeof(app->status), "Server: Already initialized");
        // ecs_singleton_modified(it->world, libevent_context_t);
        return;
    }

    struct event_base *ev_base = event_base_new();
    if (!ev_base) {
        snprintf(app->status, sizeof(app->status), "Server: Failed to create event base");
        // ecs_singleton_modified(it->world, libevent_context_t);
        return;
    }

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(libevent_server->port); // Use uint16_t port

    struct evconnlistener *listener = evconnlistener_new_bind(
        ev_base, server_accept_cb, app,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
        (struct sockaddr *)&sin, sizeof(sin)
    );
    if (!listener) {
        event_base_free(ev_base);
        snprintf(app->status, sizeof(app->status), "Server: Failed to create listener");
        // ecs_singleton_modified(it->world, libevent_context_t);
        return;
    }

    libevent_server->is_init = true;
    libevent_server->listener = listener;

    ecs_singleton_set(it->world, libevent_base_t, {
        .ev_base = ev_base
    });
    snprintf(app->status, sizeof(app->status), "Server: Initialized on port %d", libevent_server->port);
    // ecs_singleton_modified(it->world, libevent_context_t);
}

// Server cleanup on remove
void on_remove_server(ecs_iter_t *it) {
    libevent_server_t *libevent_server = ecs_field(it, libevent_server_t, 0);
    libevent_context_t *app = ecs_singleton_get_mut(it->world, libevent_context_t);
    if (!app) return;

    if (libevent_server->listener) {
        evconnlistener_free(libevent_server->listener);
        libevent_server->listener = NULL;
    }

    libevent_base_t *libevent_base = ecs_singleton_get_mut(it->world, libevent_base_t);
    if (libevent_base && libevent_base->ev_base) {
        event_base_free(libevent_base->ev_base);
        ecs_singleton_remove(it->world, libevent_base_t);
    }

    // TODO:need to add remove event for client.
    // for (int i = 0; i < num_clients; i++) {
    //     if (connected_clients[i]) {
    //         bufferevent_free(connected_clients[i]);
    //         connected_clients[i] = NULL;
    //     }
    // }
    // num_clients = 0;

    snprintf(app->status, sizeof(app->status), "Server: Shut down");
    // ecs_singleton_modified(it->world, libevent_context_t);
}

// Client: Error callback
void client_error_cb(struct bufferevent *bev, short events, void *ctx) {
    libevent_context_t *app = (libevent_context_t *)ctx;
    if (events & BEV_EVENT_ERROR) {
        snprintf(app->status, sizeof(app->status), "Client: Error %d", EVUTIL_SOCKET_ERROR());
    } else if (events & BEV_EVENT_EOF) {
        snprintf(app->status, sizeof(app->status), "Client: Connection closed");
        bufferevent_free(bev);
        // ecs_singleton_remove(app->world, libevent_client_t);
        // ecs_singleton_modified(app->world, libevent_context_t);
    }
}

// Client setup on add
void on_setup_client(ecs_iter_t *it) {
    libevent_context_t *app = ecs_singleton_get_mut(it->world, libevent_context_t);
    libevent_client_t *libevent_client = ecs_field(it, libevent_client_t, 0);
    // libevent_client_t *libevent_client = ecs_singleton_get_mut(it->world, libevent_client_t);
    
    if (!app) {
        ecs_singleton_set(it->world, libevent_context_t, {
            .world = it->world,
            .pings_sent = 0,
            .pongs_received = 0,
            .status = "Client: Initializing..."
        });
        app = ecs_singleton_get_mut(it->world, libevent_context_t);
    }

    if (libevent_client->is_init) {
        snprintf(app->status, sizeof(app->status), "Client: Already initialized");
        printf("Client: Already initialized\n");
        // ecs_singleton_modified(it->world, libevent_context_t);
        return;
    }

    struct event_base *ev_base = event_base_new();
    if (!ev_base) {
        snprintf(app->status, sizeof(app->status), "Client: Failed to create event base");
        printf("Client: Failed to create event base\n");
        // ecs_singleton_modified(it->world, libevent_context_t);
        return;
    }

    struct bufferevent *client_bev = bufferevent_socket_new(ev_base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!client_bev) {
        event_base_free(ev_base);
        snprintf(app->status, sizeof(app->status), "Client: Failed to create bufferevent");
        printf("Client: Failed to create bufferevent\n");
        // ecs_singleton_modified(it->world, libevent_context_t);
        return;
    }

    printf("port: %d\n", libevent_client->port);
    printf("address: %s\n", libevent_client->address);

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    // server_addr.sin_port = htons(SERVER_PORT);
    // int ip_status = inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr);
    
    server_addr.sin_port = htons(libevent_client->port);
    if (inet_pton(AF_INET, libevent_client->address, &server_addr.sin_addr) <= 0) {
        bufferevent_free(client_bev);
        event_base_free(ev_base);
        snprintf(app->status, sizeof(app->status), "Client: Invalid server address '%s'", libevent_client->address);
        printf("Client: Invalid server address '%s'", libevent_client->address);
        // ecs_singleton_modified(it->world, libevent_context_t);
        return;
    }

    bufferevent_setcb(client_bev, client_read_cb, NULL, client_error_cb, app);
    bufferevent_enable(client_bev, EV_READ | EV_WRITE);

    if (bufferevent_socket_connect(client_bev, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        bufferevent_free(client_bev);
        event_base_free(ev_base);
        snprintf(app->status, sizeof(app->status), "Client: Connection failed");
        printf("Client: Connection failed\n");
        // ecs_singleton_modified(it->world, libevent_context_t);
        return;
    }

    ecs_singleton_set(it->world, libevent_base_t, {
        .ev_base = ev_base
    });
    libevent_client->client_bev = client_bev;
    libevent_client->is_init = true;
    snprintf(app->status, sizeof(app->status), "Client: Connected to %s:%d", libevent_client->address, libevent_client->port);
    // ecs_singleton_modified(it->world, libevent_context_t);
    printf("client set up finished.");
}

// enet packet for string data
void on_receive_libevent_packed(ecs_iter_t *it) {
    printf("event on_receive_libevent_packed\n");
    libevent_packet_t *p = it->param;
    if (p && p->data) {
        char *str = (char*)p->data;
        printf("Received string: %s\n", str);
        free(str); // Free the dynamically allocated string
    } else {
        printf("No valid string data received\n");
    }
}

// enet packet for string data
// nope it crashed if over lap
// void on_receive_libevent_packed2(ecs_iter_t *it) {
//     printf("event on_receive_libevent_packed2\n");
//     libevent_packet_t *p = it->param;
//     if (p && p->data) {
//         char *str = (char*)p->data;
//         printf("Received string: %s\n", str);
//         free(str); // Free the dynamically allocated string
//     } else {
//         printf("No valid string data received\n");
//     }
// }

// Client cleanup on remove
void on_remove_client(ecs_iter_t *it) {
    libevent_client_t *libevent_client = ecs_field(it, libevent_client_t, 0);
    libevent_context_t *app = ecs_singleton_get_mut(it->world, libevent_context_t);
    if (!app) return;

    if (libevent_client->client_bev) {
        bufferevent_free(libevent_client->client_bev);
        libevent_client->client_bev = NULL;
    }

    libevent_base_t *libevent_base = ecs_singleton_get_mut(it->world, libevent_base_t);
    if (libevent_base && libevent_base->ev_base) {
        event_base_free(libevent_base->ev_base);
        ecs_singleton_remove(it->world, libevent_base_t);
    }

    libevent_client->is_init = false;
    snprintf(app->status, sizeof(app->status), "Client: Shut down");
    ecs_singleton_modified(it->world, libevent_context_t);
}

// systems
void setup_systems_libevent(ecs_world_t *world){

    // loop if add for Singleton
    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "event_base_system", .add = ecs_ids(ecs_dependson(PreLogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(libevent_base_t), .src.id = ecs_id(libevent_base_t) } // Singleton
        },
      .callback = event_base_system
    });

    // Create observer that is invoked whenever server is added
    ecs_observer(world, {
        .query.terms = {{ 
            // ecs_id(libevent_server_t) 
            .id = ecs_id(libevent_server_t), .src.id = ecs_id(libevent_server_t)
        }},
        .events = { EcsOnSet },
        .callback = on_add_setup_server
    });

    // Create observer that is invoked whenever server is remove
    ecs_observer(world, {
        .query.terms = {{ ecs_id(libevent_server_t) }},
        .events = { EcsOnRemove },
        .callback = on_remove_server
    });

    // Create observer that is invoked whenever client is added
    ecs_observer(world, {
        .query.terms = {{ 
            // ecs_id(libevent_client_t) 
            .id = ecs_id(libevent_client_t), .src.id = ecs_id(libevent_client_t)
        }},
        .events = { EcsOnSet }, // EcsOnAdd this does not for Singleton
        .callback = on_setup_client
    });

    // Create observer that is invoked whenever client is remove
    ecs_observer(world, {
        .query.terms = {{ ecs_id(libevent_client_t) }},
        .events = { EcsOnRemove },
        .callback = on_remove_client
    });

    // Create an entity observer for packed
    ecs_observer(world, {
        // Not interested in any specific component
        .query.terms = {{ EcsAny, .src.id = libevent_receive_packed }},
        .events = { ecs_id(libevent_packet_t) },
        .callback = on_receive_libevent_packed
    });


    // ecs_observer(world, {
    //     // Not interested in any specific component
    //     .query.terms = {{ EcsAny, .src.id = libevent_receive_packed }},
    //     .events = { ecs_id(libevent_packet_t) },
    //     .callback = on_receive_libevent_packed2
    // });

}
// components
void setup_components_libevent(ecs_world_t *world){
    ECS_COMPONENT_DEFINE(world, libevent_base_t); 
    ECS_COMPONENT_DEFINE(world, libevent_server_t); 
    ECS_COMPONENT_DEFINE(world, libevent_client_t); 
    ECS_COMPONENT_DEFINE(world, libevent_context_t); 
    ECS_COMPONENT_DEFINE(world, libevent_bev_t);
    ECS_COMPONENT_DEFINE(world, libevent_packet_t);

    // Define the event entity network type
    libevent_receive_packed = ecs_entity(world, { .name = "libevent_receive_packed" });
}

void module_init_libevent(ecs_world_t *world){
    setup_components_libevent(world);
    setup_systems_libevent(world);
}