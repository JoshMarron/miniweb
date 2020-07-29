#ifndef INCLUDED_CONNECTION_MANAGER_H
#define INCLUDED_CONNECTION_MANAGER_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct connection_manager connection_manager_t;

connection_manager_t* connection_manager_create(size_t initial_capacity);
int                   connection_manager_init(connection_manager_t* restrict conns,
                                              size_t                         initial_capacity);
void connection_manager_add_listener_socket(connection_manager_t* restrict conns,
                                            int                            sockfd);

bool connection_manager_get_next_event(connection_manager_t* manager,
                                       int*                  sockfd_out);

int connection_manager_add_new_connection(connection_manager_t* manager, int sockfd);

void connection_manager_remove_connection(connection_manager_t* manager, int sockfd);

void connection_manager_clean(connection_manager_t* restrict conns);
void connection_manager_destroy(connection_manager_t* restrict conns);

#endif // INCLUDED_CONNECTION_MANAGER_H
