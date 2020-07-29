add_library(miniweb
            logging.c
            connection_manager.c
            server.c)

add_executable(miniweb.tsk
               main.c)

target_link_libraries(miniweb.tsk PRIVATE miniweb)
