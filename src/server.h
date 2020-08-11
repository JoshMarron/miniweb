#ifndef INCLUDED_SERVER_H
#define INCLUDED_SERVER_H

#include "router.h"

typedef struct miniweb_server miniweb_server_t;

// this will take ownership of the router - the user is not responsible for
// destroying it
miniweb_server_t* miniweb_server_create(char const* const address,
                                        char const* const port,
                                        router_t*         router);
int               miniweb_server_init(miniweb_server_t* restrict server,
                                      char const* const          address,
                                      char const* const          port,
                                      router_t*                  router);

int miniweb_server_start(miniweb_server_t* restrict server);
// Should set a variable on the server to get it to stop polling
void miniweb_server_stop(miniweb_server_t* restrict server);

void miniweb_server_clean(miniweb_server_t* restrict server);
void miniweb_server_destroy(miniweb_server_t* restrict server);

#endif // INCLUDED_SERVER_H
