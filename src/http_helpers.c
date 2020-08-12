#include "http_helpers.h"

#include "logging.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <sys/fcntl.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>

// ==== CONSTANTS ====

static char const ERROR_404_RESPONSE_FILE[] = "res/404.html";

static char const OK_HEADER_TEMPLATE[] =
    "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: text/html\r\nConnection: "
    "keep-alive\r\nKeep-Alive: timeout=300\r\nContent-Length: %zu\r\n\r\n";

static char const   DATE_FORMAT[] = "%a, %d %b %Y %T GMT";
static const size_t DATE_BUF_SIZE = 40;

static const size_t OK_HEADER_BUF_SIZE =
    sizeof(OK_HEADER_TEMPLATE) + DATE_BUF_SIZE + 10;

// ==== STATIC PROTOTYPES ====

static const char* get_now_string(size_t bufsize, char buffer[bufsize]);
static off_t       get_html_filesize(int file_fd);
static int send_header(int sockfd, size_t data_size, char const data[data_size]);
static int send_html_file(int sockfd, int file_fd, off_t filesize);

// ==== PUBLIC FUNCTIONS IMPLEMENTATION ====

int http_helpers_send_response(int sockfd, miniweb_response_t const* const response)
{
    assert(response);

    switch (response->resp_type)
    {
        case MINIWEB_RESPONSE_FILE_TYPE:
            return http_helpers_send_html_file_response(
                sockfd, response->body.file_response.file_name);
        case MINIWEB_RESPONSE_TEXT_TYPE:
            MINIWEB_LOG_ERROR("Text response not yet supported!");
            return http_helpers_send_html_file_response(sockfd,
                                                        ERROR_404_RESPONSE_FILE);
        default:
            MINIWEB_LOG_ERROR("Invalid response attempted! (type: %d)",
                              response->resp_type);
            return http_helpers_send_html_file_response(sockfd,
                                                        ERROR_404_RESPONSE_FILE);
    }
}

int http_helpers_send_html_file_response(int sockfd, char const filename[static 1])
{
    assert(filename);

    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1)
    {
        MINIWEB_LOG_ERROR("Failed to open() file at '%s': %d (%s)", filename, errno,
                          strerror(errno));
        errno = 0;
        return -1;
    }

    char time_buf[DATE_BUF_SIZE]    = {0};
    char buffer[OK_HEADER_BUF_SIZE] = {0};
    int  msg_size = snprintf(buffer, OK_HEADER_BUF_SIZE, OK_HEADER_TEMPLATE,
                            get_now_string(DATE_BUF_SIZE, time_buf),
                            get_html_filesize(file_fd));
    if (msg_size < 0)
    {
        MINIWEB_LOG_ERROR("Failed to format header! rc: %d", msg_size);
        return -2;
    }
    MINIWEB_LOG_INFO("Formatted header: %s", buffer);

    int rc = send_header(sockfd, msg_size, buffer);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to send header: %d", rc);
        return -3;
    }

    rc = send_html_file(sockfd, file_fd, get_html_filesize(file_fd));
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to send file: %d", rc);
        return -4;
    }

    return 0;
}

char const* http_helpers_get_route(char const request[static 1],
                                   size_t     buflen,
                                   char       buffer[buflen])
{
    assert(request);
    assert(buffer);

    // Find the start of the route
    char* route_start = strchr(request, '/');
    if (!route_start)
    {
        MINIWEB_LOG_ERROR("Invalid request received, no route found! Request: %s",
                          request);
        return NULL;
    }

    // Now find the end of that route
    char* route_end = strchr(route_start, ' ');
    if (!route_end)
    {
        MINIWEB_LOG_ERROR(
            "Invalid request received, can't find route end! Request: %s", request);
        return NULL;
    }

    size_t route_len = route_end - route_start;
    if (route_len >= buflen)
    {
        MINIWEB_LOG_ERROR(
            "Buffer of size %zu is not big enough for route of len %zu!\n", buflen,
            route_len);
        return NULL;
    }

    strncpy(buffer, route_start, route_len);

    return buffer;
}

// ==== STATIC FUNCTION IMPLEMENTATIONS ====

static const char* get_now_string(size_t bufsize, char buffer[bufsize])
{
    time_t     now    = time(NULL);
    struct tm* now_tm = gmtime(&now);
    strftime(buffer, bufsize, DATE_FORMAT, now_tm);

    return buffer;
}

static off_t get_html_filesize(int file_fd)
{
    struct stat stat_out;
    int         rc = fstat(file_fd, &stat_out);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to stat file, rc: %d", rc);
        return 0;
    }

    return stat_out.st_size;
}

static int send_header(int sockfd, size_t data_size, char const data[data_size])
{
    assert(data);
    char const* data_ptr = data;
    while (data_size > 0)
    {
        int bytes_sent = send(sockfd, data_ptr, data_size, 0);
        if (bytes_sent <= 0)
        {
            MINIWEB_LOG_ERROR("Failed to send header to socket %d: %d (%s)", sockfd,
                              errno, strerror(errno));
            return -1;
        }

        data_size -= bytes_sent;
        data_ptr += bytes_sent;
    }

    return 0;
}

static int send_html_file(int sockfd, int file_fd, off_t filesize)
{
    off_t offset = 0;
    while (filesize > 0)
    {
        ssize_t bytes_sent = sendfile(sockfd, file_fd, &offset, filesize);
        if (bytes_sent <= 0)
        {
            MINIWEB_LOG_ERROR("Failed to send file at %d to socket %d: %d (%s)",
                              file_fd, sockfd, errno, strerror(errno));
            errno = 0;
            return -1;
        }

        filesize -= bytes_sent;
    }

    return 0;
}
