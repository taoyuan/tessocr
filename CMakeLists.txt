cmake_minimum_required(VERSION 3.1)

project(tessocr)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-cmake)

find_package(NodeJS)

list(APPEND LIB_PATHS /usr/local/include /usr/include)

include_directories(LIB_PATHS)
nodejs_find_module_fallback(nan ${CMAKE_CURRENT_SOURCE_DIR} NAN_PATH)
include_directories(${NAN_PATH})

add_nodejs_module(
    ${PROJECT_NAME}
    src/helpers.h
    src/tessocr.cc
)

find_library(LEPT NAMES "lept" PATHS ${LIB_PATHS})
find_library(TESSERACT NAMES "tesseract" PATHS ${LIB_PATHS})
target_link_libraries(${PROJECT_NAME} ${LEPT} ${TESSERACT})