cmake_minimum_required(VERSION 3.25)
project(gl_textrenderer)

set(CMAKE_CXX_STANDARD 23)

include_directories(vendor)

add_executable(${PROJECT_NAME}
        vendor/stb_image/stb_image.cpp
        vendor/stb_truetype/stb_truetype.cpp
        main.cpp
        )

find_package ( glfw3 REQUIRED )
target_link_libraries(${PROJECT_NAME} PUBLIC glfw )

find_package ( glbinding REQUIRED )
target_link_libraries(${PROJECT_NAME} PUBLIC glbinding::glbinding )

# make glfw work with glbinding
target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_NONE)