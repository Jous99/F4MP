cmake_minimum_required(VERSION 3.20)
project(F4MPServer LANGUAGES CXX)

add_executable(F4MPServer
    src/main.cpp
    src/Server.cpp
    src/ClientManager.cpp
    src/WorldState.cpp
)

target_include_directories(F4MPServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(F4MPServer PRIVATE
    f4mp_common
)

target_compile_features(F4MPServer PRIVATE cxx_std_20)

set_target_properties(F4MPServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)
