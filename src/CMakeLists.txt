add_library(miniweb
            logging.c
            server.c)

add_executable(miniweb.tsk
               main.c)

target_link_libraries(miniweb.tsk PRIVATE miniweb)
