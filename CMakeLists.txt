cmake_minimum_required(VERSION 3.23)

project(ParticleSim)
file(GLOB particleFiles src/*.cpp)
add_executable(ParticleSim ${particleFiles})
target_compile_options(ParticleSim PRIVATE -pthread)
set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)

target_link_libraries(ParticleSim PRIVATE webgpu_dawn webgpu_glfw glfw)
