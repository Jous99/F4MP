cmake_minimum_required(VERSION 3.9)
project(F4MPSim CXX)

if (NOT WIN32)
    message(FATAL_ERROR "El simulador solo compila en Windows por ahora.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencias vendidas en el repo (third_party), sin vcpkg.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/deps/x64-windows/share")
    list(INSERT CMAKE_PREFIX_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/deps/x64-windows")
endif()

find_package(GameNetworkingSockets CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(F4MPSim src/main.cpp)

target_link_libraries(F4MPSim
    GameNetworkingSockets::GameNetworkingSockets
    spdlog::spdlog
    ws2_32
    crypt32
)
