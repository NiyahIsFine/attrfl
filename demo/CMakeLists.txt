cmake_minimum_required(VERSION 3.20)
project(demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE attrfl_SRC ../runtime/src/*.cpp)
add_library(attrfl STATIC ${attrfl_SRC})
target_include_directories(attrfl PUBLIC ../runtime/include)

file(GLOB_RECURSE mySer_SRC Serialization/src/*.cpp)
add_library(mySer STATIC ${mySer_SRC})
target_include_directories(mySer PUBLIC Serialization/include ../rapidjson/include)
target_link_libraries(mySer PUBLIC attrfl)

add_library(libA INTERFACE)
target_include_directories(libA INTERFACE libA/include)
target_link_libraries(libA INTERFACE mySer)

file(GLOB_RECURSE libB_SRC libB/src/*.cpp)
add_library(libB STATIC ${libB_SRC})
target_include_directories(libB PUBLIC libB/include)
target_link_libraries(libB PUBLIC libA)

file(GLOB_RECURSE libC_SRC libC/src/*.cpp)
add_library(libC STATIC ${libC_SRC})
target_include_directories(libC PUBLIC libC/include)
target_link_libraries(libC PRIVATE mySer)

add_executable(demo_exe main.cpp)
target_link_libraries(demo_exe PRIVATE libB libC)

if (EXISTS "${CMAKE_SOURCE_DIR}/bin/attrfl.exe" AND EXISTS "${CMAKE_SOURCE_DIR}/bin/libclang.dll")
    set(ATTRFL_EXE "${CMAKE_SOURCE_DIR}/bin/attrfl.exe")
else ()
    message(FATAL_ERROR "attrfl.exe not exists")
endif ()


include(${CMAKE_SOURCE_DIR}/../runtime/cmake/Attrfl.cmake)
set(ATTRFL_MULTITHREAD ON)
set(ATTRFL_NOSTDINC ON)

attrfl_gen(libA demo_exe)
attrfl_gen(libB demo_exe)
attrfl_gen(libC demo_exe)
attrfl_finalize()
attrfl_base_register(demo_exe)