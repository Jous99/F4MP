cmake_minimum_required(VERSION 3.9)
project(F4MPClient CXX)

set(EXECUTABLE F4MPClient)
set(SRC_DIR ./src)

# Setting resource path
add_definitions(-D_RESOURCES="${CMAKE_CURRENT_SOURCE_DIR}/resources/")

# --- Dependencias via vcpkg ---
find_package(GameNetworkingSockets CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# OJO: NO metemos ./include en la ruta de includes a proposito.
# Ahi quedan cabeceras viejas de GNS/spdlog que taparian a las de vcpkg.
include_directories(
	${SRC_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

file(GLOB SourceFiles LIST_DIRECTORIES false
	${SRC_DIR}/*
	${SRC_DIR}/Utils/*
	${SRC_DIR}/ThirdParty/*
	${SRC_DIR}/Hooks/*
	${SRC_DIR}/imgui/*
	${SRC_DIR}/Network/*
)

add_subdirectory(Common)

add_library(${EXECUTABLE} SHARED ${SourceFiles})

set_target_properties(${EXECUTABLE} PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS ON
)

target_link_libraries(${EXECUTABLE}
	${CMAKE_CURRENT_SOURCE_DIR}/lib/steam_api64.lib
	${CMAKE_CURRENT_SOURCE_DIR}/lib/detours.lib
	GameNetworkingSockets::GameNetworkingSockets
	spdlog::spdlog
	Common
	d3d11.lib
	dxgi.lib
)
