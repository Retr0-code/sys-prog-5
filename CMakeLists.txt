cmake_minimum_required(VERSION 3.17.0)

project(guess-number VERSION 0.1)

# Configure compiler
set(DIR_INCLUDE "include")
set(DIR_SOURCE "src")

option(PIPE2 "Enables pipe IPC" OFF)

# Server variables
set(SERVER_SRC
    ${DIR_SOURCE}/game/game.c
    ${DIR_SOURCE}/game/game_server.c
    ${DIR_SOURCE}/game/game_client.c
    ${DIR_SOURCE}/game/protocol.c
)

if (${PIPE2})
    add_compile_definitions(PIPE2_MESSAGING)
endif()

add_compile_options(-Wno-incompatible-pointer-types -Wno-discarded-qualifiers)

add_library(${PROJECT_NAME}_objs STATIC ${SERVER_SRC})
target_include_directories(${PROJECT_NAME}_objs PUBLIC ${DIR_INCLUDE})
add_executable(${PROJECT_NAME} ${DIR_SOURCE}/main.c)

target_include_directories(${PROJECT_NAME} PUBLIC ${DIR_INCLUDE})
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)
