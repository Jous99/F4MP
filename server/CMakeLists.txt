cmake_minimum_required(VERSION 3.7)
project(F4MPServer)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CMakeModules/)

set(executable F4MPServer)
set(SrcDIR ./src)
set(IncludeDIR ./include)

add_definitions(-D_RESOURCES="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
message(${CMAKE_CURRENT_SOURCE_DIR}/resources)

execute_process(COMMAND git log --pretty=format:'%H' -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)

add_definitions(-DGIT_VERSION="${GIT_REV}")

string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M")
add_definitions(-DBUILD_TIME="${BUILD_TIME}")

add_definitions(-DVERSION_MAJOR="1")
add_definitions(-DVERSION_MINOR="0")
add_definitions(-DVERSION_PATCH="0")

if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif(UNIX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (WIN32)
    include_directories(${executable})
endif (WIN32)

if (UNIX)
    include_directories(${executable})
endif (UNIX)

include_directories(${executable}
    ${IncludeDIR}
)

file(GLOB SourceFiles
    ${SrcDIR}/*
    ${SrcDIR}/ThirdParty/*
    ${SrcDIR}/ThirdParty/nlohmann*
    ${SrcDIR}/Network/*
)

add_executable(${executable} ${SourceFiles})

set_target_properties(${executable} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS ON
)

if (UNIX)
    target_link_libraries(${executable}
    )
endif (UNIX)

if (WIN32)
    target_link_libraries(${executable}
    )
endif (WIN32)

target_link_libraries(${executable}
    ${CMAKE_DL_LIBS}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/GameNetworkingSockets_s.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/spdlogd.lib
)
