#include "thread_pool.h"

#include "itc_queue.h"
#include "logging.h"

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include <mqueue.h>
#include <pthread.h>
#include <sys/stat.h>

// ==== STATIC CONSTANTS ====

enum
{
    WRITER_MQ_FLAGS        = O_WRONLY | O_CREAT | O_EXCL,
    READER_MQ_FLAGS        = O_RDONLY,
    MESSAGE_BUF_SIZE_BYTES = 1024,
};

enum mq_data_type
{
    MQ_FUNC_EXEC,
    MQ_EXIT
};

struct mq_func_exec
{
    thread_pool_func* func;
    void*             user_data;
};

union mq_data_union
{
    struct mq_func_exec func_exec;
};

struct mq_data
{
    enum mq_data_type   type;
    union mq_data_union data;
};

struct thread_pool_thread_state
{
    size_t    thread_num;
    pthread_t id;
    itc_queue_t* job_queue;
    // WARNING! THIS WILL BECOME INVALID AFTER ALL THREADS HAVE
    // STARTED RUNNING. It will be DELETED!
    pthread_mutex_t* started_lock;
    pthread_cond_t* started;
    bool             is_started;
};

struct thread_pool
{
    struct thread_pool_thread_state* threads;
    size_t                           num_threads;
    itc_queue_t*                     job_queue;
};

// ==== STATIC FUNCTIONS ====

void* thread_pool_job(void* data)
{
    struct thread_pool_thread_state* state = data;

    // Signal that we've started, and successfully!
    pthread_mutex_lock(state->started_lock);
    state->is_started = true;
    pthread_cond_signal(state->started);
    pthread_mutex_unlock(state->started_lock);

    unsigned char buffer[MESSAGE_BUF_SIZE_BYTES] = {0};
    for (;;)
    {
        int rc = receive_message(state->job_queue, MESSAGE_BUF_SIZE_BYTES, buffer);
        if (rc != 0)
        {
            MINIWEB_LOG_ERROR("Failed to receive message.");
            continue;
        }

        struct mq_data* message = (struct mq_data*) &buffer;

        switch (message->type)
        {
            case MQ_EXIT:
            {
                MINIWEB_LOG_INFO("Thread %zu is exiting", state->thread_num);
                return NULL;
            }
            case MQ_FUNC_EXEC:
            {
                message->data.func_exec.func(message->data.func_exec.user_data);
                break;
            }
            default:
            {
                MINIWEB_LOG_ERROR("Invalid message type received: %d",
                                  message->type);
            }
        }
    }
}

int thread_pool_stop(struct thread_pool* pool)
{
    bool all_sent = true;
    for (size_t i = 0; i < pool->num_threads; ++i)
    {
        struct mq_data message = {.type = MQ_EXIT};
        int            rc      = post_message(pool->job_queue, sizeof(message),
                              (unsigned char*) &message);
        if (rc != 0)
        {
            MINIWEB_LOG_ERROR("Failed to send exit message: %d", rc);
            all_sent = false;
        }
    }

    if (!all_sent)
    {
        MINIWEB_LOG_ERROR(
            "Could not send all threads to exit - will not try to join!");
        return -1;
    }

    // Now that we've sent all the exit notifications, we can join on the threads
    for (size_t i = 0; i < pool->num_threads; ++i)
    {
        int rc = pthread_join(pool->threads[i].id, NULL);
        if (rc != 0)
        {
            MINIWEB_LOG_ERROR(
                "Failed to stop thread %zu! Program may not exit correctly! rc: %d",
                i, rc);
        }
    }

    return 0;
}

static int thread_pool_start(struct thread_pool* restrict pool)
{
    pthread_mutex_t* mutexes = calloc(pool->num_threads, sizeof(pthread_mutex_t));
    if (!mutexes)
    {
        MINIWEB_LOG_ERROR("Could not allocate memory for mutexes");
        return -1;
    }

    pthread_cond_t* conds = calloc(pool->num_threads, sizeof(pthread_cond_t));
    if (!conds)
    {
        MINIWEB_LOG_ERROR("Could not allocate memory for condition variables");
        free(mutexes);
        return -1;
    }

    for (size_t i = 0; i < pool->num_threads; ++i)
    {
        // Initialise the condition variable for this thread
        int rc = pthread_cond_init(&conds[i], NULL);
        if (rc != 0)
        {
            MINIWEB_LOG_ERROR(
                "Failed to initialise the condition variable for thread %zu: %d", i,
                rc);
            return -2;
        }

        rc = pthread_mutex_init(&mutexes[i], NULL);
        if (rc != 0)
        {
            MINIWEB_LOG_ERROR("Failed to initialise the mutex for thread %zu: %d", i,
                              rc);
            return -2;
        }

        struct thread_pool_thread_state* state = &pool->threads[i];
        state->thread_num                      = i;
        state->started                         = &conds[i];
        state->started_lock                    = &mutexes[i];
        state->job_queue                       = pool->job_queue;
        rc = pthread_create(&state->id, NULL, thread_pool_job, state);
        if (rc != 0) { MINIWEB_LOG_ERROR("Failed to create thread number %zu!", i); }
    }

    // Now we need to wait on the condition variables
    for (size_t i = 0; i < pool->num_threads; ++i)
    {
        pthread_mutex_lock(&mutexes[i]);
        while (!pool->threads[i].is_started)
        {
            pthread_cond_wait(&conds[i], &mutexes[i]);
        }
        pthread_mutex_unlock(&mutexes[i]);

        pthread_cond_destroy(&conds[i]);
        pthread_mutex_destroy(&mutexes[i]);
    }

    free(conds);
    free(mutexes);

    return 0;
}

// ==== PUBLIC FUNCTIONS IMPLEMENTATION

struct thread_pool* thread_pool_init(size_t num_threads)
{
    struct thread_pool_thread_state* thread_storage =
        calloc(num_threads, sizeof(struct thread_pool_thread_state));
    if (!thread_storage)
    {
        MINIWEB_LOG_ERROR("Failed to allocate %zu bytes for pthread id storage",
                          num_threads * sizeof(pthread_t));
        return NULL;
    }

    struct thread_pool* pool = calloc(1, sizeof(struct thread_pool));
    if (!pool)
    {
        MINIWEB_LOG_ERROR(
            "Failed to allocate space for thread pool control structure");
        free(thread_storage);
        return NULL;
    }
    pool->threads = thread_storage;
    pool->num_threads = num_threads;

    itc_queue_t* queue = itc_queue_init(200, sizeof(struct mq_data));
    if (!queue)
    {
        MINIWEB_LOG_ERROR("Failed to initialise message queue");
        free(thread_storage);
        free(pool);
        return NULL;
    }
    pool->job_queue = queue;

    // Start the threads
    thread_pool_start(pool);

    return pool;
}

void thread_pool_destroy(struct thread_pool* restrict pool)
{
    thread_pool_stop(pool);
    itc_queue_destroy(pool->job_queue);

    free(pool->threads);
    free(pool);
}

int thread_pool_run(struct thread_pool* restrict pool,
                    thread_pool_func*            func,
                    void*                        data)
{
    struct mq_data message = {
        .type = MQ_FUNC_EXEC,
        .data = (union mq_data_union) {
            .func_exec = (struct mq_func_exec) {.func = func, .user_data = data}}};

    int rc =
        post_message(pool->job_queue, sizeof(message), (unsigned char*) &message);

    return rc;
}
