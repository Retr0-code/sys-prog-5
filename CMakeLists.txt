cmake_minimum_required(VERSION 3.17.0)

project(guess-number VERSION 0.1)

# Configure compiler
set(DIR_INCLUDE "include")
set(DIR_SOURCE "src")

# Server variables
set(SERVER_SRC
    ${DIR_SOURCE}/server/server.c
    ${DIR_SOURCE}/server/client.c
    ${DIR_SOURCE}/game/game.c
    ${DIR_SOURCE}/game/game_server.c
    ${DIR_SOURCE}/game/protocol.c
    ${DIR_SOURCE}/sockutils.c
)

set(CLIENT_SRC
    ${DIR_SOURCE}/client/client.c
    ${DIR_SOURCE}/game/protocol.c
    ${DIR_SOURCE}/game/game.c
    ${DIR_SOURCE}/sockutils.c
)

option(USE_IPV6 "Use IPv6 for server host" OFF)
if (USE_IPV6)
  add_definitions(-DUSE_IPV6)
endif()

add_compile_options(-Wno-incompatible-pointer-types -Wno-discarded-qualifiers)

add_library(${PROJECT_NAME}_server_objs STATIC ${SERVER_SRC})
target_include_directories(${PROJECT_NAME}_server_objs PUBLIC ${DIR_INCLUDE})
add_executable(${PROJECT_NAME}-server ${DIR_SOURCE}/server/main.c)

target_include_directories(${PROJECT_NAME}-server PUBLIC ${DIR_INCLUDE})
target_link_libraries(${PROJECT_NAME}-server ${PROJECT_NAME}_server_objs)

add_library(${PROJECT_NAME}_client_objs STATIC ${CLIENT_SRC})
target_include_directories(${PROJECT_NAME}_client_objs PUBLIC ${DIR_INCLUDE})
add_executable(${PROJECT_NAME}-client ${DIR_SOURCE}/client/main.c)

target_include_directories(${PROJECT_NAME}-client PUBLIC ${DIR_INCLUDE})
target_link_libraries(${PROJECT_NAME}-client ${PROJECT_NAME}_client_objs)
