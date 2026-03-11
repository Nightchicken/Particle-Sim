cmake_minimum_required(VERSION 3.23)
project(ParticleSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_FILES src/*.cpp)
add_executable(ParticleSim ${SRC_FILES})

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)

target_link_libraries(ParticleSim PRIVATE webgpu_dawn webgpu_cpp webgpu_glfw glfw)
