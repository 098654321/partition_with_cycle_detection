cmake_minimum_required(VERSION 3.15)
project(verilog2dag_dag LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(verilog2dag
    src/main.cpp
    src/Json.cpp
    src/YosysModel.cpp
    src/DAG.cpp
    src/DAGBuilder.cpp
)

target_include_directories(verilog2dag PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

