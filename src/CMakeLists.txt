add_library(miniweb
            hash.c
            http_helpers.c
            logging.c
            connection_manager.c
            pool.c
            router.c
            miniweb_response.c
            thread_pool.c
            server.c)

add_library(miniweb-test
            logging.c
            hash.c
            router.c
            miniweb_response.c
            pool.c)

target_compile_options(miniweb PUBLIC -Wall -Wextra -pedantic -Werror)
target_compile_options(miniweb-test PRIVATE -DMINIWEB_TESTING)

add_executable(miniweb.tsk
               main.c)

target_link_libraries(miniweb.tsk PRIVATE miniweb)
