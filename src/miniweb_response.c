#include "miniweb_response.h"

#include "logging.h"

#include <assert.h>
#include <string.h>

static const size_t PRINT_BUFFER_SIZE = 2048;

// ==== STATIC FUNCTIONS ====

static char* const
miniweb_text_response_to_string(miniweb_response_t const* const response,
                                size_t                          buflen,
                                char                            buffer[buflen])
{
    assert(response);
    assert(buffer);

    int chars_would_write =
        snprintf(buffer, buflen, "{MINIWEB_TEXT_RESPONSE: body: `%s`}",
                 response->body.text_response.text_response);

    if (chars_would_write >= buflen)
    { MINIWEB_LOG_ERROR("Output was truncated when writing text_response"); }

    return buffer;
}

static char* const
miniweb_file_response_to_string(miniweb_response_t const* const response,
                                size_t                          buflen,
                                char                            buffer[buflen])
{
    assert(response);
    assert(buffer);

    int chars_would_write =
        snprintf(buffer, buflen, "{MINIWEB_FILE_RESPONSE: file_name: `%s`}",
                 response->body.file_response.file_name);

    if (chars_would_write >= buflen)
    { MINIWEB_LOG_ERROR("Output was truncated when writing file_response"); }

    return buffer;
}

miniweb_response_t miniweb_build_text_response(char const* const text_body)
{
    assert(text_body);

    miniweb_response_body body = {0};
    strncpy(body.text_response.text_response, text_body,
            MINIWEB_RESPONSE_MAX_TEXT_RESPONSE_SIZE - 1);

    return (miniweb_response_t) {.resp_type = MINIWEB_RESPONSE_TEXT_TYPE,
                                 .body      = body};
}

miniweb_response_t miniweb_build_file_response(char const* const file_name)
{
    assert(file_name);

    miniweb_response_body body = {0};
    strncpy(body.file_response.file_name, file_name,
            MINIWEB_RESPONSE_MAX_FILENAME_SIZE - 1);

    return (miniweb_response_t) {.resp_type = MINIWEB_RESPONSE_FILE_TYPE,
                                 .body      = body};
}

void miniweb_print_response(miniweb_response_t const* const response, FILE* output)
{
    assert(output);
    assert(response);

    char print_buffer[PRINT_BUFFER_SIZE];
    fprintf(output, "%s\n",
            miniweb_response_to_string(response, PRINT_BUFFER_SIZE, print_buffer));
}

char* const miniweb_response_to_string(miniweb_response_t const* const response,
                                       size_t                          buflen,
                                       char buffer[buflen])
{
    assert(response);
    assert(buffer);

    switch (response->resp_type)
    {
        case MINIWEB_RESPONSE_TEXT_TYPE:
            return miniweb_text_response_to_string(response, buflen, buffer);
        case MINIWEB_RESPONSE_FILE_TYPE:
            return miniweb_file_response_to_string(response, buflen, buffer);
        default:
            MINIWEB_LOG_ERROR("Invalid response type: %d", response->resp_type);
            snprintf(buffer, buflen, "{INVALID RESPONSE}");
            return buffer;
    }
}
