#ifndef INCLUDED_HTTP_PARSER_H
#define INCLUDED_HTTP_PARSER_H

#include <stdlib.h>

int parse_http_header(size_t data_size, char const data[data_size]);

#endif // INCLUDED_HTTP_PARSER_H
