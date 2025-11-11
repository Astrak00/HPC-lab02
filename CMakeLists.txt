cmake_minimum_required(VERSION 3.11)

project(
  contrast
  VERSION 1.0
  LANGUAGES CXX)


find_package(MPI)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    message(STATUS "OpenMP C flags: ${OpenMP_C_FLAGS}")
    message(STATUS "OpenMP CXX flags: ${OpenMP_CXX_FLAGS}")
    message(STATUS "OpenMP EXE linker flags: ${OpenMP_EXE_LINKER_FLAGS}")
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

message(STATUS "${CMAKE_CXX_FLAGS}")


include_directories(SYSTEM ${MPI_INCLUDE_PATH})

add_executable(contrast contrast-enhancement.cpp histogram-equalization.cpp  contrast.cpp)

target_link_libraries(contrast ${MPI_C_LIBRARIES})
