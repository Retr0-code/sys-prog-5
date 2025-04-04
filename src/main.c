#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "game/game_server.h"
#include "game/game_client.h"

#define MAX_ATTEMPTS 5
#define GAME_ROUNDS 10

void graceful_stop(int singal)
{
}

int main(int argc, char **argv)
{
    typedef int (*game_role_t)(size_t max_attempts);
    game_role_t role = &game_run_server;
    pid_t pid = fork();
    switch (pid)
    {
        case -1:
            perror("Error while forking");
            break;
        case 0:
            role = &game_run_client;
            break;
    }

    signal(SIGINT, &graceful_stop);
    signal(SIGTERM, &graceful_stop);

    (*role)(MAX_ATTEMPTS);

    return 0;
}
