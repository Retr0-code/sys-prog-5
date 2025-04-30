#pragma once
#include "game/game.h"

int game_run_client(int serverfd, int clientfd, size_t max_tries);
