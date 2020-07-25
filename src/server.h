#ifndef INCLUDED_SERVER_H
#define INCLUDED_SERVER_H

typedef struct miniweb_server miniweb_server_t;

miniweb_server_t* server_create();
void              server_init(miniweb_server_t* restrict server);

#endif // INCLUDED_SERVER_H
