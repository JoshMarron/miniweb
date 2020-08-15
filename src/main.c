#include "logging.h"
#include "router.h"
#include "server.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>

miniweb_server_t* g_server = NULL;

void sig_handler(int signum)
{
    (void) signum;
    miniweb_server_stop(g_server);
}

miniweb_response_t default_route_handler(void* user_data, char const request[const])
{
    (void) user_data;
    MINIWEB_LOG_INFO("Received request: %s", request);

    return miniweb_build_file_response("res/index.html");
}

miniweb_response_t hello_route_handler(void* user_data, char const request[const])
{
    assert(request);
    (void) user_data;

    return miniweb_build_file_response("res/hello.html");
}

int main(void)
{
    router_t* router = router_init();
    if (!router)
    {
        MINIWEB_LOG_ERROR("Failed to create router!");
        return EXIT_FAILURE;
    }

    int rc = router_add_route(router, "/", default_route_handler, NULL);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to add route for '/', rc: %d", rc);
        router_destroy(router);
        return EXIT_FAILURE;
    }

    rc = router_add_route(router, "/hello", hello_route_handler, NULL);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to add route for '/hello', rc: %d", rc);
        router_destroy(router);
        return EXIT_FAILURE;
    }

    g_server = miniweb_server_create("127.0.0.1", "6969", router);
    if (!g_server)
    {
        MINIWEB_LOG_ERROR("Failed to create a server!");
        router_destroy(router);
        return EXIT_FAILURE;
    }

    // Setup the termination handler
    struct sigaction signal_action = {0};
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_handler = sig_handler;
    sigaction(SIGINT, &signal_action, NULL);
    sigaction(SIGTERM, &signal_action, NULL);

    MINIWEB_LOG_INFO("I AM MINIWEB, STARTING SERVER!");
    rc = miniweb_server_start(g_server);

    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Server did not exit cleanly, rc: %d", rc);
        miniweb_server_destroy(g_server);
        return EXIT_FAILURE;
    }

    miniweb_server_destroy(g_server);
    MINIWEB_LOG_INFO("Goodbye!");
    return EXIT_SUCCESS;
}
