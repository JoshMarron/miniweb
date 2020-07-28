#include "server.h"

#include "connection_manager.h"
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

struct miniweb_server
{
    int                        sock_fd;
    atomic_bool                should_run;
    connection_manager_t*      connections;
};

// ==== STATIC PROTOTYPES ====

static int miniweb_server_get_bound_socket(char const* const address,
                                           char const* const port);
static int miniweb_server_listen(struct miniweb_server* server);

// TODO implement these:
static int miniweb_server_handle_new_connection(struct miniweb_server* server);
static int miniweb_server_process_client_event(struct miniweb_server* server,
                                               int                    connection_fd);

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

    connection_manager_t* conns = connection_manager_create(INITIAL_SERVER_CAPACITY);
    if (!conns)
    {
        MINIWEB_LOG_ERROR("Failed to create connection_manager struct");
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
    connection_manager_add_listener_socket(server->connections, server->sock_fd);

    // And enter our main loop!
    server->should_run = true;
    return miniweb_server_listen(server);
}

void miniweb_server_stop(miniweb_server_t* server)
{
    server->should_run = false;
}

void miniweb_server_clean(miniweb_server_t* restrict server)
{
    assert(server);

    connection_manager_destroy(server->connections);
    memset(server, 0, sizeof(struct miniweb_server));
}

void miniweb_server_destroy(miniweb_server_t* restrict server)
{
    if (server) { miniweb_server_clean(server); }
    free(server);
}

// ==== STATIC FUNCTIONS IMPLEMENTATION ====

static int miniweb_server_listen(struct miniweb_server* server)
{
    server->should_run = true;
    for (;;)
    {
        int recv_fd = -1;
        while (connection_manager_get_next_event(server->connections, &recv_fd))
        {
            if (recv_fd == server->sock_fd)
            {
                // NEW CONNECTION
                miniweb_server_handle_new_connection(server);
            }
            else
            {
                // NEW MESSAGE FROM CONNECTED CLIENT
                miniweb_server_process_client_event(server, recv_fd);
            }
        }

        if (!server->should_run)
        {
            MINIWEB_LOG_ERROR("Server for socket %d is stopping!", server->sock_fd);
            break;
        }
    }

    // We've successfully exited
    return 0;
}

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

