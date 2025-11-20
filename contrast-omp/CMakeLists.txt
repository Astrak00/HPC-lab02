cmake_minimum_required(VERSION 3.11)

project(
  contrast
  VERSION 1.0
  LANGUAGES CXX)

# --- Find OpenMP (this part was working) ---
find_package(OpenMP)
if (OPENMP_FOUND)
  message(STATUS "OpenMP found, will add flags.")
else()
  message(WARNING "OpenMP not found.")
endif()

# --- Define Target ---
add_executable(
  contrast
  contrast-enhancement.cpp
  histogram-equalization.cpp
  contrast.cpp)

# --- Add Optimization Flags ---
target_compile_options(contrast PRIVATE -O3 -march=native)

# --- Manually Add MPI Settings ---
# We are hard-coding what 'mpicxx.mpich -show' told us.
# No find_package(MPI)!

# Add Include Directory: -I/usr/include/x86_64-linux-gnu/mpich
target_include_directories(contrast PUBLIC /usr/include/x86_64-linux-gnu/mpich)

# Add Link Directory: -L/usr/lib/x86_64-linux-gnu
target_link_directories(contrast PUBLIC /usr/lib/x86_64-linux-gnu)

# Add Linker Flag: -Wl,-z,relro
target_link_options(contrast PUBLIC "-Wl,-z,relro")

# Add Link Libraries: -lmpichcxx -lmpich
target_link_libraries(contrast PUBLIC mpichcxx mpich ucp ucs hwloc pthread rt)
# --- Link OpenMP ---
# If OpenMP was found, link it too
if (OPENMP_FOUND)
  target_link_libraries(contrast PUBLIC OpenMP::OpenMP_CXX)
endif()
