#include "server.h"

#include "logging.h"

#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>

// CONSTANTS

static const size_t INITIAL_SERVER_CAPACITY = 10;
static const int    MAX_QUEUED_CONNECTIONS  = 10;

// ==== STRUCTS ====

struct active_connections
{
    struct pollfd* connections;
    size_t         connections_num;
    size_t         connections_cap;
};

struct miniweb_server
{
    int                        sock_fd;
    atomic_bool                should_run;
    struct active_connections* connections;
};

// ==== STATIC PROTOTYPES ====

static int miniweb_server_get_bound_socket(char const* const address,
                                           char const* const port);

static struct active_connections* active_connections_create(size_t initial_capacity);
static int  active_connections_init(struct active_connections* restrict conns,
                                    size_t initial_capacity);
static void
            active_connections_add_listener_socket(struct active_connections* restrict conns,
                                                   int                                 sockfd);
static void active_connections_clean(struct active_connections* restrict conns);
static void active_connections_destroy(struct active_connections* restrict conns);

// ==== PUBLIC FUNCTIONS IMPLEMENTATION ====

struct miniweb_server* miniweb_server_create(char const* const address,
                                             char const* const port)
{
    struct miniweb_server* server = calloc(1, sizeof(struct miniweb_server));
    if (!server)
    {
        MINIWEB_LOG_ERROR("Failed to allocate memory for miniweb_server");
        return NULL;
    }

    int rc = miniweb_server_init(server, address, port);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to initialise miniweb_server, rc: %d", rc);
        free(server);
        return NULL;
    }

    return server;
}

int miniweb_server_init(miniweb_server_t* restrict server,
                        char const* const          address,
                        char const* const          port)
{
    assert(server);

    struct active_connections* conns =
        active_connections_create(INITIAL_SERVER_CAPACITY);
    if (!conns)
    {
        MINIWEB_LOG_ERROR("Failed to create active_connections struct");
        return -1;
    }
    server->connections = conns;

    int socket = miniweb_server_get_bound_socket(address, port);
    if (socket == -1)
    {
        MINIWEB_LOG_ERROR(
            "Failed to connect to get a bound socket for address: %s and port: %s",
            address, port);
        miniweb_server_clean(server);
        return -1;
    }

    server->sock_fd = socket;

    return 0;
}

int miniweb_server_start(miniweb_server_t* server)
{
    assert(server);

    int rc = listen(server->sock_fd, MAX_QUEUED_CONNECTIONS);
    if (rc == -1)
    {
        MINIWEB_LOG_ERROR("Server failed to start listening on main socket: %d (%s)",
                          errno, strerror(errno));
        errno = 0;
        return -1;
    }

    // Add the listening socket to our maintained connections for polling
    active_connections_add_listener_socket(server->connections, server->sock_fd);

    // And enter our main loop!
    server->should_run = true;
    for (;;)
    {
        // TODO: WRITE THE ACTUAL RUN LOOP

        if (!server->should_run)
        {
            MINIWEB_LOG_ERROR("Server for socket %d is stopping!", server->sock_fd);
            break;
        }
    }

    return 0;
}

void miniweb_server_stop(miniweb_server_t* server)
{
    server->should_run = false;
}

void miniweb_server_clean(miniweb_server_t* restrict server)
{
    assert(server);

    active_connections_destroy(server->connections);
    memset(server, 0, sizeof(struct miniweb_server));
}

void miniweb_server_destroy(miniweb_server_t* restrict server)
{
    if (server) { miniweb_server_clean(server); }
    free(server);
}

// ==== STATIC FUNCTIONS IMPLEMENTATION ====

static int miniweb_server_get_bound_socket(char const* const address,
                                           char const* const port)
{
    assert(address);
    assert(port);

    struct addrinfo hints = {0};
    hints.ai_family       = AF_UNSPEC;
    hints.ai_socktype     = SOCK_STREAM;
    hints.ai_flags        = AI_PASSIVE;

    struct addrinfo* info = NULL;

    int rc = getaddrinfo(address, port, &hints, &info);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR(
            "Failed to get address info for address: %s and port: %s, rc: %d",
            address, port, rc);
        return -1;
    }

    for (struct addrinfo* p = info; p != NULL; p = p->ai_next)
    {
        int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            MINIWEB_LOG_ERROR(
                "Failed to get socket for address: %s and port: %s (%s)", address,
                port, strerror(errno));
            errno = 0;
            continue;
        }

        int bind_rc = bind(sockfd, info->ai_addr, info->ai_addrlen);
        if (rc == -1)
        {
            MINIWEB_LOG_ERROR(
                "Failed to bind socket %d for address: %s and port: %s (%s)", sockfd,
                address, port, strerror(errno));
            errno = 0;
            continue;
        }

        MINIWEB_LOG_INFO(
            "Successfully bound socket %d for address: %s and port: %s !", sockfd,
            address, port);
        freeaddrinfo(info);
        return sockfd;
    }

    // If we get here, we failed to get anything
    MINIWEB_LOG_ERROR(
        "Failed to get or bind any sockets for address: %s and port %s !", address,
        port);
    freeaddrinfo(info);
    return -1;
}

static struct active_connections* active_connections_create(size_t initial_capacity)
{
    struct active_connections* conns = calloc(1, sizeof(struct active_connections));
    if (!conns)
    {
        MINIWEB_LOG_ERROR("Failed to allocate memory for active_connections");
        return NULL;
    }

    int rc = active_connections_init(conns, initial_capacity);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to initialise active_connections, rc: %d", rc);
        free(conns);
        return NULL;
    }

    return conns;
}

static int active_connections_init(struct active_connections* restrict conns,
                                   size_t initial_capacity)
{
    assert(conns);

    struct pollfd* connections = calloc(initial_capacity, sizeof(struct pollfd));
    if (!connections)
    {
        MINIWEB_LOG_ERROR("Failed to allocate memory for pollfd array");
        return -1;
    }

    conns->connections     = connections;
    conns->connections_num = 0;
    conns->connections_cap = initial_capacity;

    return 0;
}

static void
active_connections_add_listener_socket(struct active_connections* restrict conns,
                                       int                                 sockfd)
{
    assert(conns);

    conns->connections[0].fd     = sockfd;
    conns->connections[0].events = POLLIN;
}

static void active_connections_clean(struct active_connections* restrict conns)
{
    assert(conns);

    free(conns->connections);
    memset(conns, 0, sizeof(struct active_connections));
}

static void active_connections_destroy(struct active_connections* restrict conns)
{
    if (conns) { active_connections_clean(conns); }
    free(conns);
}
