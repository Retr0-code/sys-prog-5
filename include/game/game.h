#pragma once
#include <stdlib.h>

#include "game/protocol.h"

typedef struct
{
    int bottom;
    int top;
} range_t;

typedef struct
{
    range_t range;
    size_t  tries;
} game_client_settings_t;

typedef struct
{
    range_t range;
    size_t  tries;
    int     answer;
    int     guess;
} guess_number_t;

int game_send_client_settings(int client_fd, const game_client_settings_t *settings);

int game_receive_client_settings(int client_fd, game_client_settings_t *settings);

int game_send_guess(int client_fd, int guess);

int game_receive_guess(int client_fd, int *guess);

int game_send_answer(int client_fd, int answer);

int game_receive_answer(int client_fd, int *answer);
