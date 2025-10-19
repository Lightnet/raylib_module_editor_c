#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <event.h>
#include <evutil.h>

#define PORT 5555
#define TEST_MESSAGE "Hello, Libevent!"
#define MAX_LINE 256

// Server callback: Handle incoming connections
static void accept_conn_cb(int listen_fd, short ev, void *arg) {
    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listen_fd, (struct sockaddr *)&ss, &slen);
    if (fd < 0) {
        perror("accept");
        return;
    }
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
    }

    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    printf("Server: Accepted connection\n");
}

// Server callback: Echo received data
static void echo_read_cb(struct bufferevent *bev, void *ctx) {
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    evbuffer_add_buffer(output, input); // Echo back
    printf("Server: Echoed data\n");
}

// Server callback: Handle connection errors or closure
static void echo_event_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_ERROR) {
        perror("Server: Connection error");
        bufferevent_free(bev);
    }
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
        printf("Server: Connection closed\n");
    }
}

// Client callback: Handle received data
static void client_read_cb(struct bufferevent *bev, void *ctx) {
    struct evbuffer *input = bufferevent_get_input(bev);
    char *response = malloc(evbuffer_get_length(input) + 1);
    evbuffer_remove(input, response, evbuffer_get_length(input));
    response[evbuffer_get_length(input)] = '\0';

    const char *expected = TEST_MESSAGE;
    if (strcmp(response, expected) == 0) {
        printf("Test passed: Received echoed '%s'\n", response);
    } else {
        printf("Test failed: Expected '%s', got '%s'\n", expected, response);
    }
    free(response);

    bufferevent_free(bev);
    event_base_loopexit((struct event_base *)ctx, NULL); // Exit after test
}

// Client callback: Handle connection errors or closure
static void client_error_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_ERROR) {
        perror("Client: Connection error");
    }
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
        event_base_loopexit((struct event_base *)ctx, NULL);
    }
}

// Client callback: Handle connection establishment
static void client_connect_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_CONNECTED) {
        printf("Client: Connected to server\n");
        evbuffer_add_printf(bufferevent_get_output(bev), "%s\n", TEST_MESSAGE);
        bufferevent_enable(bev, EV_READ | EV_WRITE);
    }
}

int main(void) {
    // Initialize Libevent
    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "Failed to create event base\n");
        return 1;
    }

    // Server setup
    struct sockaddr_in sin;
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }
    evutil_make_socket_nonblocking(listen_fd);

    int one = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(listen_fd, 16) < 0) {
        perror("listen");
        return 1;
    }

    struct event *listen_ev = event_new(base, listen_fd, EV_READ | EV_PERSIST, accept_conn_cb, base);
    event_add(listen_ev, NULL);
    printf("Server: Listening on port %d\n", PORT);

    // Client setup
    struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, client_read_cb, NULL, client_error_cb, base);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    evutil_inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);

    if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("Client: Connection failed");
        bufferevent_free(bev);
        return 1;
    }
    printf("Client: Initiating connection\n");

    // Run the event loop
    event_base_dispatch(base);

    // Cleanup
    event_base_free(base);
    close(listen_fd);
    printf("Test completed\n");
    return 0;
}