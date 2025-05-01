#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "game/game_server.h"
#include "game/game_client.h"

#include "game/game.h"

#define MAX_ATTEMPTS 5
#define GAME_ROUNDS 10

#define SEMAPHORE_NAME "/guess_num_game"

void graceful_stop(int singal)
{
    semaphore_close(SEMAPHORE_NAME);
    exit(0);
}

void signals_add_handlers(void)
{
    signal(SIGINT, &graceful_stop);
    signal(SIGTERM, &graceful_stop);
}

int main(int argc, char **argv)
{
    typedef int (*game_role_t)(pid_t, pid_t, size_t);
    game_role_t role = &game_run_server;
    // game_role_t role = &game_run_client;
    pid_t ppid = getpid();

    sigset_init();

    // pid_t cpid = 0;
    pid_t cpid = fork();
    switch (cpid)
    {
    case -1:
        perror("Error while forking");
        return -1;
    case 0:
        role = &game_run_client;
        break;
    }
    signals_add_handlers();
    if (semaphore_init(SEMAPHORE_NAME, 0) == -1)
    {
        perror("semaphore_init");
        graceful_stop(0);
        return -1;
    }
    return (*role)(ppid, cpid, MAX_ATTEMPTS);
}
