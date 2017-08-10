set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra")

add_executable(graphics main.c graphics.c collision.c)
add_executable(test_aabb test_aabb.c graphics.c collision.c)
target_link_libraries(graphics glad glfw lodepng)
target_link_libraries(test_aabb glad glfw lodepng)
include_directories(../include)
