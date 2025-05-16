#pragma once
#include "game/game.h"

void game_set_top_limit(int max);

int game_run_server(int serverfd, int clientfd, size_t max_tries, game_stats_t *stats);
