#ifndef INCLUDED_HTTP_HELPERS_H
#define INCLUDED_HTTP_HELPERS_H

#include <stdlib.h>

int http_helpers_send_html_file_response(int sockfd, char const filename[static 1]);

#endif // INCLUDED_HTTP_HELPERS_H
