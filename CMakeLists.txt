cmake_minimum_required(VERSION 3.20)
project(Termen LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configuración de Raylib
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(raylib)

set(SOURCES
    src/main.cpp
    src/app.cpp
    src/font_system.cpp
    src/glossary.cpp
    src/text_utils.cpp
    src/config_manager.cpp
)

set(HEADERS
    include/app.hpp
    include/font_system.hpp
    include/glossary.hpp
    include/text_utils.hpp
    include/config_manager.hpp
)

add_executable(Termen WIN32 ${SOURCES} ${HEADERS})

target_include_directories(Termen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${raylib_SOURCE_DIR}/src
)

target_link_libraries(Termen PRIVATE raylib)

if(WIN32)
    target_link_libraries(Termen PRIVATE winmm gdi32 opengl32 shell32)
endif()
