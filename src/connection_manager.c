#include "connection_manager.h"

#include "logging.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <poll.h>

static const int CONNECTION_MANAGER_DEFAULT_TIMEOUT = 2000;

struct connection_manager
{
    struct pollfd* connections;
    size_t         connections_num;
    size_t         connections_cap;
    int            current_event_index;
};

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
}

bool connection_manager_get_next_event(connection_manager_t* manager,
                                       int*                  sockfd_out)
{
    assert(sockfd);
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

        // Let's start returning events!
        manager->current_event_index = 0;
    }

    for (int i = manager->current_event_index; i < manager->connections_num; ++i)
    {
        if (manager->connections[i].revents & POLLIN)
        {
            *sockfd_out = manager->connections[i].fd;
            manager->current_event_index++;
            return true;
        }
    }

    // If we get here then we are out of polling events and should set our index to
    // -1
    manager->current_event_index = -1;
    return false;
}

void connection_manager_clean(struct connection_manager* restrict conns)
{
    assert(conns);

    free(conns->connections);
    memset(conns, 0, sizeof(struct connection_manager));
}

void connection_manager_destroy(struct connection_manager* restrict conns)
{
    if (conns) { connection_manager_clean(conns); }
    free(conns);
}
