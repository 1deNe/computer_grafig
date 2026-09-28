cmake_minimum_required(VERSION 3.20)
project(cg LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT cg)

# --- GLFW және GLM: CMake оларды GitHub-тан өзі жүктейді ---
include(FetchContent)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)  # қосымша пакеттер керек болмасын

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4 GIT_SHALLOW TRUE)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glfw glm)

find_package(OpenGL REQUIRED)

# --- Бүкіл бағдарлама: бір ғана файл ---
add_executable(cg
    main.cpp
    external/glad/src/gl.c
)

target_include_directories(cg PRIVATE
    external/glad/include
    external/stb
)

target_link_libraries(cg PRIVATE OpenGL::GL glfw glm::glm)

# Ассеттерді кодта табу үшін (11-аптадан бастап керек)
target_compile_definitions(cg PRIVATE
    ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)



# ==========================================
# 2-БАҒДАРЛАМА — second.cpp
# ==========================================

add_executable(second
    second.cpp
    external/glad/src/gl.c
)

target_include_directories(second PRIVATE
    external/glad/include
    external/stb
)

target_link_libraries(second PRIVATE
    OpenGL::GL
    glfw
    glm::glm
)

target_compile_definitions(second PRIVATE
    ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)
