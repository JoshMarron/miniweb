add_executable(miniweb.t
               main.t.c
               pool.t.c
               hash.t.c
               thread_pool.t.c
               router.t.c)

# Disable unused parameter warning in test drivers. because I don't care!
target_compile_options(miniweb.t PRIVATE -Wall -Wextra -pedantic -Werror -Wno-unused-parameter)

find_package(cmocka CONFIG REQUIRED)
target_include_directories(miniweb.t PRIVATE ${CMOCKA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(miniweb.t PRIVATE ${CMOCKA_LIBRARIES} miniweb-test)
