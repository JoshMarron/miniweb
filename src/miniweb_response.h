#ifndef INCLUDED_MINIWEB_RESPONSE_H
#define INCLUDED_MINIWEB_RESPONSE_H

#include <stdio.h>
#include <stdlib.h>

enum
{
    MINIWEB_RESPONSE_MAX_TEXT_RESPONSE_SIZE = 1024,
    MINIWEB_RESPONSE_MAX_FILENAME_SIZE      = 256,
};

enum miniweb_response_type
{
    MINIWEB_RESPONSE_TEXT_TYPE,
    MINIWEB_RESPONSE_FILE_TYPE
};

typedef struct
{
    char text_response[MINIWEB_RESPONSE_MAX_TEXT_RESPONSE_SIZE];
} miniweb_text_response_t;

typedef struct
{
    char file_name[MINIWEB_RESPONSE_MAX_FILENAME_SIZE];
} miniweb_file_response_t;

typedef union
{
    miniweb_text_response_t text_response;
    miniweb_file_response_t file_response;
} miniweb_response_body;

typedef struct
{
    enum miniweb_response_type resp_type;
    miniweb_response_body      body;
} miniweb_response_t;

miniweb_response_t miniweb_build_text_response(char const* const text_body);
miniweb_response_t miniweb_build_file_response(char const* const file_name);

void miniweb_print_response(miniweb_response_t const* const response, FILE* output);
char* const miniweb_response_to_string(miniweb_response_t const* const response,
                                       size_t                          buflen,
                                       char buffer[buflen]);

#endif
