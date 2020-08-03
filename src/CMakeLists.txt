add_library(miniweb
            http_helpers.c
            logging.c
            connection_manager.c
            pool.c
            server.c)

add_executable(miniweb.tsk
               main.c)

target_link_libraries(miniweb.tsk PRIVATE miniweb)
