cmake_minimum_required(VERSION 3.15)
project(c-gen LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  afranchuk_prettyprint
  GIT_REPOSITORY https://github.com/afranchuk/prettyprint
  GIT_TAG        master
)
FetchContent_MakeAvailable(afranchuk_prettyprint)

add_library(afranchuk_prettyprint_lib STATIC 
    ${afranchuk_prettyprint_SOURCE_DIR}/src/prettyprint.c
    ${afranchuk_prettyprint_SOURCE_DIR}/src/prettyprintcpp.cpp
)

target_compile_definitions(afranchuk_prettyprint_lib PRIVATE PRETTYPRINT_CPP_INTERNAL=1)

target_include_directories(afranchuk_prettyprint_lib PUBLIC 
    ${afranchuk_prettyprint_SOURCE_DIR}/src
)

find_package(pugixml REQUIRED)

set(SOURCES
    src/main.cpp
    src/xml_parser.cpp
    src/transformer.cpp
    src/codewriter.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    pugixml
    afranchuk_prettyprint_lib
)

if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
