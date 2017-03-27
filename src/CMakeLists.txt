set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")

add_executable(graphics main.c)
target_link_libraries(graphics glad glfw)
