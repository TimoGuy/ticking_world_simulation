cmake_minimum_required(VERSION 3.28)

project(ticking_world_simulation
    VERSION 0.1.0
    LANGUAGES CXX
)

# C/C++ standard.
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Multithreaded build.
set(CMAKE_BUILD_PARALLEL_LEVEL 16)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

# Dependencies.
add_subdirectory(third_party/JoltPhysics/Build)

# Static library build.
add_library(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/simulating_entity_ifc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/world_simulation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/world_simulation.h
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jolt_physics
        ${multithreaded_job_system_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    Jolt
)
