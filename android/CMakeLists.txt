project(NitroPdf)
cmake_minimum_required(VERSION 3.9.0)

set (PACKAGE_NAME NitroPdf)
set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 20)

set (SHARED_CPP_DIR ${CMAKE_SOURCE_DIR}/../cpp)
set (LITEHTML_DIR ${SHARED_CPP_DIR}/third_party/litehtml)
set (GUMBO_DIR ${LITEHTML_DIR}/src/gumbo)

file(GLOB LITEHTML_SOURCES ${LITEHTML_DIR}/src/*.cpp)
file(GLOB GUMBO_SOURCES ${GUMBO_DIR}/*.c)
add_library(litehtml STATIC ${LITEHTML_SOURCES} ${GUMBO_SOURCES})
set_target_properties(litehtml PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(litehtml
        PUBLIC ${LITEHTML_DIR}/include
        PRIVATE ${LITEHTML_DIR}/src ${LITEHTML_DIR}/include/litehtml ${GUMBO_DIR}/include ${GUMBO_DIR}/include/gumbo
)
target_compile_options(litehtml PRIVATE -w)

file(GLOB NITROPDF_SOURCES ${SHARED_CPP_DIR}/*.cpp ${SHARED_CPP_DIR}/core/*.cpp)
add_library(${PACKAGE_NAME} SHARED
        src/main/cpp/cpp-adapter.cpp
        src/main/cpp/SystemFonts.cpp
        ${NITROPDF_SOURCES}
)

include(${CMAKE_SOURCE_DIR}/../nitrogen/generated/android/NitroPdf+autolinking.cmake)

include_directories(
        "src/main/cpp"
        "../cpp"
)

find_library(LOG_LIB log)

target_link_libraries(
        ${PACKAGE_NAME}
        ${LOG_LIB}
        android
        litehtml
        z
)
