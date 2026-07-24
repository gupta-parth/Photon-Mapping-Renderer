cmake_minimum_required(VERSION 3.16)
project(CS488)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(external/glew)
add_subdirectory(external/glfw)

file(GLOB SRC_FILES
        src/*.h
        src/*.cpp)

add_executable(CS488
        ${SRC_FILES})

target_include_directories(CS488 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)
target_include_directories(CS488 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glew/include)
target_link_libraries(CS488 glfw libglew_static)

add_executable(test_photon_map
        tests/test_photon_map.cpp)

target_include_directories(test_photon_map PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(test_photon_map PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)
target_include_directories(test_photon_map PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glew/include)
