project(sim)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_compile_definitions(ARCH_CM0)
add_compile_definitions(DEBUG_MODE)

aux_source_directory(src/bus BUS)
aux_source_directory(src/cpu CPU)
aux_source_directory(src/common COM)

add_executable(${PROJECT_NAME} ${BUS} ${CPU} ${COM})