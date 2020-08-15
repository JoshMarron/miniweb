#ifndef INCLUDED_ITC_QUEUE_H
#define INCLUDED_ITC_QUEUE_H

#include <stdlib.h>

typedef struct itc_queue itc_queue_t;

itc_queue_t* itc_queue_init(size_t max_size, size_t msglen);

int          post_message(itc_queue_t* restrict queue,
                          size_t                msglen,
                          unsigned char const   msg[msglen]);
int          post_message_noblock(itc_queue_t* restrict queue,
                                  size_t                msglen,
                                  unsigned char const   msg[msglen]);
int          receive_message(itc_queue_t* restrict queue,
                             size_t                buflen,
                             unsigned char         buf[buflen]);
int          receive_message_noblock(itc_queue_t* restrict queue,
                                     size_t                buflen,
                                     unsigned char         buf[buflen]);

void itc_queue_destroy(itc_queue_t* restrict queue);

#endif // INCLUDED_ITC_QUEUE_H
