#ifndef INCLUDED_HTTP_HELPERS_H
#define INCLUDED_HTTP_HELPERS_H

#include "miniweb_response.h"

#include <stdlib.h>

int http_helpers_send_response(int sockfd, miniweb_response_t const* const response);
int http_helpers_send_html_file_response(int sockfd, char const filename[static 1]);

char const* http_helpers_get_route(char const request[static 1],
                                   size_t     buflen,
                                   char       buffer[buflen]);

#endif // INCLUDED_HTTP_HELPERS_H
