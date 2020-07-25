#include "server.h"

#include "logging.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>

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
    struct active_connections* connections;
};

// ==== STATIC PROTOTYPES ====

static struct active_connections* active_connections_create();
static void active_connections_init(struct active_connections* restrict conns);
static void active_connections_clean(struct active_connections* restrict conns);
static void active_connections_destroy(struct active_connections* restrict conns);

// ==== PUBLIC FUNCTIONS IMPLEMENTATION ====

struct miniweb_server* miniweb_server_create()
{
    struct active_connections* conns = active_connections_create();
    if (!conns)
    {
        MINIWEB_LOG_ERROR("Failed to create active_connections struct");
        return NULL;
    }

    return NULL;
}

// ==== STATIC FUNCTIONS IMPLEMENTATION ====
