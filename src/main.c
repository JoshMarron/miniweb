#include "logging.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char* argv[argc + 1])
{
    MINIWEB_LOG_INFO("I AM MINIWEB");
    printf("I AM MINIWEB!\n");

    struct addrinfo hints        = {0};
    hints.ai_family              = AF_UNSPEC;
    hints.ai_socktype            = SOCK_STREAM;
    hints.ai_flags               = AI_PASSIVE;
    struct addrinfo* server_info = NULL;

    int rc = getaddrinfo(NULL, "6969", &hints, &server_info);
    if (rc != 0)
    {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(rc));
        return EXIT_FAILURE;
    }

    int s = socket(server_info->ai_family, server_info->ai_socktype,
                   server_info->ai_protocol);
    if (s == -1)
    {
        fprintf(stderr, "Call to socket failed!\n");
        freeaddrinfo(server_info);
        return EXIT_FAILURE;
    }

    rc = bind(s, server_info->ai_addr, server_info->ai_addrlen);
    if (rc != 0)
    {
        fprintf(stderr, "Call to bind failed: %d\n", errno);
        return EXIT_FAILURE;
    }

    freeaddrinfo(server_info);
    server_info = NULL;

    rc = listen(s, 10);
    if (rc == -1)
    {
        fprintf(stderr, "Call to listen failed: %d\n", errno);
        return EXIT_FAILURE;
    }

    struct sockaddr_storage their_addr     = {0};
    socklen_t               their_addr_len = sizeof(their_addr);

    int new_sockfd = accept(s, (struct sockaddr*) &their_addr, &their_addr_len);
    if (new_sockfd == -1)
    {
        fprintf(stderr, "Call to accept failed: %d\n", errno);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
