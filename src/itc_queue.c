#include "itc_queue.h"

#include "logging.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <pthread.h>

struct itc_queue
{
    unsigned char* storage;
    size_t         read;
    size_t         write;

    size_t capacity;
    size_t num_elements;
    size_t msglen;

    pthread_mutex_t rw_lock;
};

enum
{
    ITC_QUEUE_DEFAULT_SLEEP_NS = 10 * 1000 * 1000,
};

enum
{
    ITC_QUEUE_EQUEUEFULL  = -1,
    ITC_QUEUE_EQUEUEEMPTY = -2,
};

// ==== PUBLIC FUNCTIONS IMPLEMENTATION

struct itc_queue* itc_queue_init(size_t max_size, size_t msglen)
{
    unsigned char* storage = calloc(max_size, msglen);
    if (!storage)
    {
        MINIWEB_LOG_ERROR("Could not allocate space for %zu elements of %zu bytes",
                          max_size, msglen);
        return NULL;
    }

    struct itc_queue* queue = calloc(1, sizeof(struct itc_queue));
    if (!queue)
    {
        MINIWEB_LOG_ERROR("Could not allocate space for queue control structure");
        free(storage);
        return NULL;
    }

    queue->storage      = storage;
    queue->read         = 0;
    queue->write        = 0;
    queue->capacity     = max_size;
    queue->num_elements = 0;
    queue->msglen       = msglen;

    int rc = pthread_mutex_init(&queue->rw_lock, NULL);
    if (rc != 0)
    {
        MINIWEB_LOG_ERROR("Failed to init read_lock: %d (%s)", errno,
                          strerror(errno));
        errno = 0;
        free(queue);
        free(storage);
        return NULL;
    }

    return queue;
}

int post_message_noblock(struct itc_queue* restrict queue,
                         size_t                     msglen,
                         unsigned char const        msg[msglen])
{
    assert(queue);
    assert(msglen <= queue->msglen);

    pthread_mutex_lock(&queue->rw_lock);
    if (queue->num_elements == queue->capacity)
    {
        pthread_mutex_unlock(&queue->rw_lock);
        return ITC_QUEUE_EQUEUEFULL;
    }

    // Advance the write ptr.
    queue->write = (queue->write + 1) % queue->capacity;
    // All good to write!
    memcpy(queue->storage + (queue->write * queue->msglen), msg, msglen);
    queue->num_elements++;

    pthread_mutex_unlock(&queue->rw_lock);
    return 0;
}

int post_message(struct itc_queue* restrict queue,
                 size_t                     msglen,
                 unsigned char const        msg[msglen])
{
    assert(queue);
    assert(msglen <= queue->msglen);

    for (;;)
    {
        int rc = post_message_noblock(queue, msglen, msg);
        if (rc == ITC_QUEUE_EQUEUEFULL)
        {
            struct timespec sleep = {.tv_nsec = ITC_QUEUE_DEFAULT_SLEEP_NS};
            nanosleep(&sleep, NULL);
        }
        else
        {
            return 0;
        }
    }

}

int receive_message_noblock(struct itc_queue* restrict queue,
                            size_t                     buflen,
                            unsigned char              buf[buflen])
{
    assert(queue);
    assert(buflen >= queue->msglen);


    pthread_mutex_lock(&queue->rw_lock);

    if (queue->num_elements == 0)
    {
        // In this case the queue is empty
        pthread_mutex_unlock(&queue->rw_lock);
        return ITC_QUEUE_EQUEUEEMPTY;
    }

    // Otherwise, copy this data into our buffer, clear it, and return
    queue->read = (queue->read + 1) % queue->capacity;
    memcpy(buf, queue->storage + (queue->read * queue->msglen), queue->msglen);
    memset(queue->storage + (queue->read * queue->msglen), 0, queue->msglen);
    queue->num_elements--;

    pthread_mutex_unlock(&queue->rw_lock);

    return 0;
}

int receive_message(struct itc_queue* restrict queue,
                    size_t                     buflen,
                    unsigned char              buf[buflen])
{
    assert(queue);
    assert(buflen >= queue->msglen);

    for (;;)
    {
        int rc = receive_message_noblock(queue, buflen, buf);
        if (rc == ITC_QUEUE_EQUEUEEMPTY)
        {
            struct timespec sleep = {.tv_nsec = ITC_QUEUE_DEFAULT_SLEEP_NS};
            nanosleep(&sleep, NULL);
        }
        else
        {
            return 0;
        }
    }
}

void itc_queue_destroy(struct itc_queue* restrict queue)
{
    // Acquire both the mutexes so we know it's safe to proceed
    pthread_mutex_lock(&queue->rw_lock);

    free(queue->storage);
    queue->storage = NULL;

    pthread_mutex_unlock(&queue->rw_lock);

    pthread_mutex_destroy(&queue->rw_lock);

    free(queue);
}
