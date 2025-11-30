cmake_minimum_required(VERSION 3.23)

project(ParticleSim)
file(GLOB particleFiles src/*.cpp)
add_executable(ParticleSim ${particleFiles})
target_compile_options(ParticleSim PRIVATE -pthread)
