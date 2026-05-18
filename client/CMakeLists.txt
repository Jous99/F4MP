cmake_minimum_required(VERSION 3.20)
project(F4MPClient LANGUAGES CXX)

add_library(F4MPClient SHARED
    src/main.cpp
    src/hooks/D3D11Hook.cpp
    src/hooks/InputHook.cpp
    src/network/NetworkClient.cpp
    src/network/MessageHandler.cpp
    src/ui/OverlayUI.cpp
    src/utils/GameAddresses.cpp
)

target_include_directories(F4MPClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/f4se/f4se
    ${CMAKE_SOURCE_DIR}/f4se/f4se_common
)

target_link_libraries(F4MPClient PRIVATE
    f4mp_common
    d3d11
    dxgi
    winmm
)

target_compile_features(F4MPClient PRIVATE cxx_std_20)

set_target_properties(F4MPClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)
