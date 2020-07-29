#include "logging.h"
#include "server.h"

#include <signal.h>
#include <stdlib.h>

miniweb_server_t* g_server = NULL;

void sig_handler(int signum)
{
    miniweb_server_stop(g_server);
}

int main(int argc, char* argv[argc + 1])
{
    g_server = miniweb_server_create("127.0.0.1", "9999");
    if (!g_server)
    {
        MINIWEB_LOG_ERROR("Failed to create a server!");
        return EXIT_FAILURE;
    }

    // Setup the termination handler
    struct sigaction signal_action = {0};
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_handler       = sig_handler;
    sigaction(SIGINT, &signal_action, NULL);
    sigaction(SIGTERM, &signal_action, NULL);

    MINIWEB_LOG_INFO("I AM MINIWEB, STARTING SERVER!");
    int rc = miniweb_server_start(g_server);

    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Server did not exit cleanly, rc: %d", rc);
        return EXIT_FAILURE;
    }

    MINIWEB_LOG_INFO("Goodbye!");
    return EXIT_SUCCESS;
}
