#include "connection_manager.h"

#include "logging.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <poll.h>
#include <unistd.h>

static const int CONNECTION_MANAGER_DEFAULT_TIMEOUT = 2000;

struct connection_manager
{
    struct pollfd* connections;
    size_t         connections_num;
    size_t         connections_cap;
    int            current_event_index;
};

// ==== STATIC PROTOTYPES ====

static size_t connection_manager_find_free_spot(struct connection_manager* manager);

struct connection_manager* connection_manager_create(size_t initial_capacity)
{
    struct connection_manager* conns = calloc(1, sizeof(struct connection_manager));
    if (!conns)
    {
        MINIWEB_LOG_ERROR("Failed to allocate memory for connection_manager");
        return NULL;
    }

    int rc = connection_manager_init(conns, initial_capacity);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to initialise connection_manager, rc: %d", rc);
        free(conns);
        return NULL;
    }

    return conns;
}

int connection_manager_init(struct connection_manager* restrict conns,
                            size_t                              initial_capacity)
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
    conns->current_event_index = -1;

    return 0;
}

void connection_manager_add_listener_socket(struct connection_manager* restrict
                                                conns,
                                            int sockfd)
{
    assert(conns);

    conns->connections[0].fd     = sockfd;
    conns->connections[0].events = POLLIN;
    conns->connections_num       = 1;
}

bool connection_manager_get_next_event(connection_manager_t* manager,
                                       int*                  sockfd_out)
{
    assert(sockfd_out);
    assert(manager);

    // We're not currently iterating, and need to call poll()
    if (manager->current_event_index == -1)
    {
        int npolls = poll(manager->connections, manager->connections_num,
                          CONNECTION_MANAGER_DEFAULT_TIMEOUT);
        if (npolls == -1)
        {
            MINIWEB_LOG_ERROR("Call to poll() failed: %d (%s)", errno,
                              strerror(errno));
            errno = 0;
            return false;
        }
        if (npolls == 0)
        {
            // We timed out
            return false;
        }

        // Let's start returning events!
        manager->current_event_index = 0;
    }

    for (int i = manager->current_event_index; i < manager->connections_num; ++i)
    {
        if (manager->connections[i].revents & POLLIN)
        {
            *sockfd_out = manager->connections[i].fd;
            manager->current_event_index = i + 1;
            return true;
        }
    }

    // If we get here then we are out of polling events and should set our index to
    // -1
    manager->current_event_index = -1;
    return false;
}

int connection_manager_add_new_connection(struct connection_manager* manager,
                                          int                        new_sockfd)
{
    // Found a free spot in the current array
    size_t new_index = connection_manager_find_free_spot(manager);
    if (new_index == SIZE_MAX)
    {
        // Need to allocate a new buffer
        size_t new_size = manager->connections_cap * 2;
        struct pollfd* new_buf =
            realloc(manager->connections, new_size * sizeof(struct pollfd));
        if (!new_buf)
        {
            MINIWEB_LOG_ERROR("Was unable to allocate memory for the new buffer!");
            return -1;
        }

        // Realloc won't set the second half to 0, so we should do that ourselves
        memset(manager->connections + manager->connections_cap, 0,
               manager->connections_cap * sizeof(struct pollfd));
        new_index                = manager->connections_cap;
        manager->connections_cap = new_size;
    }

    manager->connections[new_index].fd = new_sockfd;
    manager->connections[new_index].events |= POLLIN;
    ++manager->connections_num;

    return 0;
}

void connection_manager_remove_connection(struct connection_manager* manager,
                                          int                        sockfd)
{
    // Find the socket in the connection array
    for (size_t i = 0; i < manager->connections_cap; ++i)
    {
        if (manager->connections[i].fd == sockfd)
        {
            // And zero it out
            manager->connections[i] = (const struct pollfd) {0};
            --manager->connections_num;
            return;
        }
    }

    // If we get here, then there was no such socket
    MINIWEB_LOG_ERROR("Attempted to delete socket %d but no such socket was found",
                      sockfd);
}

static size_t connection_manager_find_free_spot(struct connection_manager* manager)
{
    for (size_t i = 0; i < manager->connections_cap; ++i)
    {
        if (manager->connections[i].events == 0 && manager->connections[i].fd == 0)
        { return i; }
    }

    // If we got here, then we didn't find anything
    return SIZE_MAX;
}

void connection_manager_clean(struct connection_manager* restrict conns)
{
    assert(conns);

    for (size_t i = 0; i < conns->connections_cap; ++i)
    {
        if (conns->connections[i].events != 0) { close(conns->connections[i].fd); }
    }

    free(conns->connections);
    memset(conns, 0, sizeof(struct connection_manager));
}

void connection_manager_destroy(struct connection_manager* restrict conns)
{
    if (conns) { connection_manager_clean(conns); }
    free(conns);
}
