cmake_minimum_required(VERSION 3.1)

project(tessocr)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-cmake)

find_package(NodeJS)

include_directories(/usr/local/include)
nodejs_find_module_fallback(nan ${CMAKE_CURRENT_SOURCE_DIR} NAN_PATH)
include_directories(${NAN_PATH})

add_nodejs_module(
    ${PROJECT_NAME}
    src/helpers.h
    src/tessocr.cc
)

find_library(LEPT NAMES "lept" PATHS /usr/local/lib)
find_library(TESSERACT NAMES "tesseract" PATHS /usr/local/lib)
target_link_libraries(${PROJECT_NAME} ${LEPT} ${TESSERACT})