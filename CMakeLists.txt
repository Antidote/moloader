cmake_minimum_required(VERSION 3.28)
project(moloader)

set(CMAKE_CXX_STANDARD 23)

option(MOLOADER_WITH_EXAMPLE "Compile example (default: OFF)" OFF)

add_library(moloader STATIC
        include/moloader/moloader.hpp
        src/moloader.cpp
)

add_library(moloader::moloader ALIAS moloader)

target_include_directories(moloader PUBLIC
        include
)

if (MOLOADER_WITH_EXAMPLE)
    add_executable(moloader-example
            example/main.cpp
    )
    target_link_libraries(moloader-example PRIVATE
            moloader
    )
endif ()