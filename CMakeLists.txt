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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/jolt_physics_headers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/physics_objects.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pool_elem_key.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/simulating_ifc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/standard_behaviors.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ticking_world_simulation_public.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/world_simulation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/jolt_phys_impl__custom_listeners.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/jolt_phys_impl__error_callbacks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/jolt_phys_impl__job_system_integration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/jolt_phys_impl__job_system_integration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/jolt_phys_impl__layers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/jolt_phys_impl__obj_vs_broad_phase_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_objects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulating_ifc__factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/standard_behaviors__gamepad_input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/standard_behaviors__humanoid_movement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/world_simulation__create_jolt_physics_world.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/world_simulation.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        # ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jolt_physics  @NOTE: Remove from include dirs so that can be externally accessed.
        ${cglm_INCLUDE_DIR}
        ${input_handling_INCLUDE_DIR}
        ${multithreaded_job_system_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    Jolt
    multithreaded_job_system
)
