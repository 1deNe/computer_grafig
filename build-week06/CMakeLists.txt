cmake_minimum_required(VERSION 3.20)
project(week06 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4 GIT_SHALLOW TRUE)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glfw glm)

find_package(OpenGL REQUIRED)
get_filename_component(ARCHIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
add_executable(week06 6-week.cpp "${ARCHIVE_DIR}/external/glad/src/gl.c")
target_include_directories(week06 PRIVATE "${ARCHIVE_DIR}/external/glad/include")
target_link_libraries(week06 PRIVATE OpenGL::GL glfw glm::glm)
