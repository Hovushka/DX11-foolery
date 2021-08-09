# CMakeList.txt : CMake project for DXtest, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

find_package(SDL2 CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)
find_package(directxmath CONFIG REQUIRED)

# Add source to this project's executable.
add_executable(DXtest src/main.cpp src/d3drenderer.cpp src/window.cpp src/sprite.cpp src/cube.cpp)

set_property(TARGET DXtest PROPERTY CXX_STANDARD 20)
target_include_directories(DXtest PRIVATE src/)
target_link_libraries(DXtest PRIVATE SDL2::SDL2 d3d11.lib dxgi.lib d3dcompiler.lib Microsoft::DirectXTK Microsoft::DirectXMath)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sampleShader.hlsl ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/res/)

# TODO: Add tests and install targets if needed.
