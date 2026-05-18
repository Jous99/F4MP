cmake_minimum_required(VERSION 3.20)
project(F4MPCommon LANGUAGES CXX)

add_library(f4mp_common STATIC
    src/Logger.cpp
    src/PatternScanner.cpp
    src/Config.cpp
    src/NetworkProtocol.cpp
)

target_include_directories(f4mp_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(f4mp_common PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

target_compile_features(f4mp_common PUBLIC cxx_std_20)
