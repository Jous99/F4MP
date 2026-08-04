cmake_minimum_required(VERSION 3.9)
project(F4MPServer CXX)

# --- Solo Windows por ahora ---
if (NOT WIN32)
    message(FATAL_ERROR "F4MPServer solo compila en Windows por ahora.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_RESOURCES="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
add_definitions(-D_WINDOWS)

# --- Info de version (opcional, no rompe si git no esta) ---
execute_process(COMMAND git log --pretty=format:'%H' -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)
add_definitions(-DGIT_VERSION="${GIT_REV}")

string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M")
add_definitions(-DBUILD_TIME="${BUILD_TIME}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Fuentes reales del servidor ---
# (Se excluye a proposito el codigo legacy: Server.cpp/.hpp y Logger.cpp/.hpp)
add_executable(F4MPServer
    src/main.cpp
    src/Network/GameServer.cpp
)

# --- Librerias ---
target_link_libraries(F4MPServer
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/GameNetworkingSockets_s.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/spdlogd.lib
    ws2_32      # Winsock (lo necesita GameNetworkingSockets)
    crypt32     # Cifrado (lo necesita GameNetworkingSockets)
    advapi32    # Servicios de Windows (StartServiceCtrlDispatcher en main.cpp)
)
