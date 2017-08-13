set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

add_executable(graphics main.c graphics.c collision.c)
set_property(TARGET graphics PROPERTY C_STANDARD 11)
target_compile_options(graphics PRIVATE ${GRAPHICS_BUILD_OPTIONS})
target_link_libraries(graphics glad glfw lodepng ${GRAPHICS_MATH_LIBS})
target_include_directories(graphics PRIVATE "../include")
