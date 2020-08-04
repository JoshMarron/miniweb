add_executable(miniweb.t
               pool.t.c)

find_package(cmocka CONFIG REQUIRED)
target_include_directories(miniweb.t PRIVATE ${CMOCKA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(miniweb.t PRIVATE ${CMOCKA_LIBRARIES} miniweb)
