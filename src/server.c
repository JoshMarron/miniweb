#include "server.h"

#include "connection_manager.h"
#include "http_helpers.h"
#include "logging.h"

#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// CONSTANTS

static const size_t INITIAL_SERVER_CAPACITY = 10;
static const int    MAX_QUEUED_CONNECTIONS  = 10;

// ==== STRUCTS ====

struct miniweb_server
{
    int                   sock_fd;
    atomic_bool           should_run;
    connection_manager_t* connections;
    router_t*             router;
};

// ==== STATIC PROTOTYPES ====

static int miniweb_server_get_bound_socket(char const* const address,
                                           char const* const port);
static int miniweb_server_listen(struct miniweb_server* server);

static int miniweb_server_handle_new_connection(struct miniweb_server* server);
static int miniweb_server_process_client_event(struct miniweb_server* server,
                                               int                    connection_fd);

static void* get_sockaddr_in_from_sockaddr(struct sockaddr* sa)
{
    if (sa->sa_family == AF_INET) { return &(((struct sockaddr_in*) sa)->sin_addr); }
    else
    {
        return &(((struct sockaddr_in6*) sa)->sin6_addr);
    }
}

// ==== PUBLIC FUNCTIONS IMPLEMENTATION ====

struct miniweb_server* miniweb_server_create(char const* const address,
                                             char const* const port,
                                             router_t*         router)
{
    struct miniweb_server* server = calloc(1, sizeof(struct miniweb_server));
    if (!server)
    {
        MINIWEB_LOG_ERROR("Failed to allocate memory for miniweb_server");
        return NULL;
    }

    int rc = miniweb_server_init(server, address, port, router);
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
                        char const* const          port,
                        router_t*                  router)
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
    server->router  = router;

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

    router_destroy(server->router);
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
            MINIWEB_LOG_INFO("Got event: %d", recv_fd);
            int failed_handles = 0;
            if (recv_fd == server->sock_fd)
            {
                // NEW CONNECTION
                int rc = miniweb_server_handle_new_connection(server);
                if (rc != 0) ++failed_handles;
            }
            else
            {
                // NEW MESSAGE FROM CONNECTED CLIENT
                int rc = miniweb_server_process_client_event(server, recv_fd);
                if (rc != 0) ++failed_handles;
            }

            if (failed_handles > 0)
            { MINIWEB_LOG_ERROR("Failed to handle %d events!", failed_handles); }
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

static int miniweb_server_handle_new_connection(struct miniweb_server* server)
{
    struct sockaddr_storage their_addr = {0};
    socklen_t               addrlen    = sizeof(their_addr);

    int new_sockfd =
        accept(server->sock_fd, (struct sockaddr*) &their_addr, &addrlen);

    if (new_sockfd == -1)
    {
        MINIWEB_LOG_ERROR("Failed to accept new connection: %d (%s)", errno,
                          strerror(errno));
        errno = 0;
        return -1;
    }

    char their_addr_string[INET6_ADDRSTRLEN] = {0};
    inet_ntop(their_addr.ss_family,
              get_sockaddr_in_from_sockaddr((struct sockaddr*) &their_addr),
              their_addr_string, INET6_ADDRSTRLEN);

    int rc = connection_manager_add_new_connection(server->connections, new_sockfd);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to add new connection to the manager, rejecting"
                          "connection from %s",
                          their_addr_string);
        return -1;
    }

    MINIWEB_LOG_INFO("Accepted new connection from: %s on socket %d!",
                     their_addr_string, new_sockfd);
    return 0;
}

static int miniweb_server_process_client_event(struct miniweb_server* server,
                                               int                    connection_fd)
{
    MINIWEB_LOG_INFO("Received event on socket %d", connection_fd);

    char buffer[1024] = {0};
    int  num_bytes    = recv(connection_fd, buffer, sizeof(buffer), 0);
    if (num_bytes <= 0)
    {
        int rc = 0;
        if (num_bytes == 0)
        {
            MINIWEB_LOG_ERROR("Connection on socket %d closed by client",
                              connection_fd);
            rc = 0;
        }
        else
        {
            MINIWEB_LOG_ERROR("Error recv()ing: %d (%s)", errno, strerror(errno));
            rc = -1;
        }
        close(connection_fd);
        connection_manager_remove_connection(server->connections, connection_fd);
        return rc;
    }

    // We got some data! Let's just log it for now :)
    MINIWEB_LOG_ERROR("Got %d bytes of data from socket %d: '%s'", num_bytes,
                      connection_fd, buffer);

    char               routebuf[ROUTE_MAX_LENGTH] = {0};
    char const* route = http_helpers_get_route(buffer, sizeof(routebuf), routebuf);
    if (!route)
    {
        MINIWEB_LOG_ERROR("Failed to get route from request!");
        return -1;
    }

    miniweb_response_t response =
        router_invoke_route_func(server->router, route, buffer);

    int rc = http_helpers_send_response(connection_fd, &response);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to send response to socket %d: %d", connection_fd,
                          rc);
        return -2;
    }

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

        int yes = 1;
        int rc  = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (rc == -1)
        {
            MINIWEB_LOG_ERROR("Failed to set socket to allow address reuse: %d (%s)",
                              errno, strerror(errno));
            errno = 0;
            continue;
        }

        int bind_rc = bind(sockfd, info->ai_addr, info->ai_addrlen);
        if (bind_rc == -1)
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
