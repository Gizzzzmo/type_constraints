cmake_minimum_required(VERSION 3.18.0)
project(type_proofs LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-fdiagnostics-color=always -O2)

add_executable(main main.cpp)

add_executable(blub blub.cpp)