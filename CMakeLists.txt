cmake_minimum_required(VERSION 3.1)

project(tessocr)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-cmake)

find_package(NodeJS)

add_definitions(-DHAVE_FMEMOPEN)

add_nodejs_module(
    ${PROJECT_NAME}
    src/helpers.h
    src/tessocr.cc
)

set(LIB_PATHS /usr/local/include;/usr/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${LIB_PATHS})

find_library(LEPT NAMES "lept" PATHS ${LIB_PATHS})
find_library(TESSERACT NAMES "tesseract" PATHS ${LIB_PATHS})
target_link_libraries(${PROJECT_NAME} ${LEPT} ${TESSERACT})

#nodejs_find_module_fallback(nan ${CMAKE_CURRENT_SOURCE_DIR} NAN_PATH)
#target_include_directories(${PROJECT_NAME} PUBLIC ${LIB_PATHS} ${NAN_PATH})