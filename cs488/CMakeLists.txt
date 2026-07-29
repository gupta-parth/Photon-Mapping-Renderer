cmake_minimum_required(VERSION 3.16)
project(CS488)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(external/glew)
add_subdirectory(external/glfw)


# OpenMP
if(APPLE AND NOT DEFINED OpenMP_ROOT)
    foreach(SPPM_OPENMP_ROOT "/opt/homebrew/opt/libomp" "/usr/local/opt/libomp")
        if(EXISTS "${SPPM_OPENMP_ROOT}")
            set(OpenMP_ROOT "${SPPM_OPENMP_ROOT}" CACHE PATH "OpenMP root")
            break()
        endif()
    endforeach()
endif()
find_package(OpenMP REQUIRED)

file(GLOB SRC_FILES
        src/*.h
        src/*.cpp)

add_executable(CS488
        ${SRC_FILES})

target_include_directories(CS488 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)
target_include_directories(CS488 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glew/include)
target_link_libraries(CS488 glfw libglew_static)
target_link_libraries(CS488 OpenMP::OpenMP_CXX)

add_executable(test_photon_map
        tests/test_photon_map.cpp)

target_include_directories(test_photon_map PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(test_photon_map PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)
target_include_directories(test_photon_map PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glew/include)

add_executable(test_point_light_sampling
        tests/test_point_light_sampling.cpp)

target_include_directories(test_point_light_sampling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(test_point_light_sampling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)
target_include_directories(test_point_light_sampling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glew/include)
