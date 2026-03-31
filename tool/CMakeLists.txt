cmake_minimum_required(VERSION 3.14)
project(attrfl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set root path for llvm+clang
set(LLVM_CLANG_ROOT "E:/clang+llvm-21.1.8-x86_64-pc-windows-msvc" CACHE PATH "Optional LLVM+Clang root")

if (NOT LLVM_CLANG_ROOT)
    find_path(_CLANG_C_INC NAMES clang-c/Index.h)
    if (_CLANG_C_INC)
        get_filename_component(LLVM_CLANG_ROOT "${_CLANG_C_INC}" DIRECTORY)
    endif ()
endif ()

if (NOT LLVM_CLANG_ROOT)
    message(FATAL_ERROR "Cannot find LLVM. Set -DLLVM_CLANG_ROOT=/path/to/llvm")
endif ()

find_library(LIBCLANG_LIBRARY NAMES libclang PATHS "${LLVM_CLANG_ROOT}/lib" REQUIRED)

message(STATUS "LLVM root: ${LLVM_CLANG_ROOT}")
message(STATUS "libclang:  ${LIBCLANG_LIBRARY}")

set(CLANG_C_INCLUDE_DIR "${LLVM_CLANG_ROOT}/include")

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE ../rapidjson/include)

add_executable(attrfl main.cpp)
target_compile_definitions(attrfl PRIVATE RAPIDJSON_HAS_STDSTRING=1)

target_include_directories(attrfl PRIVATE
        "${CLANG_C_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
)
target_link_libraries(attrfl PRIVATE "${LIBCLANG_LIBRARY}" rapidjson)

if (WIN32)
    add_custom_command(TARGET attrfl POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LLVM_CLANG_ROOT}/bin/libclang.dll"
            "$<TARGET_FILE_DIR:attrfl>/libclang.dll"
    )
endif ()


#################################
target_compile_definitions(attrfl PRIVATE
        _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
)
if (MSVC)
    target_compile_options(attrfl PRIVATE /wd4996)
endif()