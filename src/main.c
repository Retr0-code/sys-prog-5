#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "game/game_server.h"
#include "game/game_client.h"

#include "status.h"
#include "game/game.h"

#define MAX_ATTEMPTS 5
#define MAX_CYCLES   10
#define GAME_ROUNDS 10

#define SEMAPHORE_NAME "/guess_num_game"

void graceful_stop(int singal)
{
    semaphore_close(SEMAPHORE_NAME);
    exit(0);
}

typedef int (*game_role_t)(pid_t, pid_t, size_t);
game_role_t game_role_swap(game_role_t current_role, pid_t *pid1, pid_t *pid2)
{
    pid_t temp = *pid1;
    *pid1 = *pid2;
    *pid2 = temp;

    if (current_role == &game_run_server)
        return &game_run_client;

    return &game_run_server;
}

void signals_add_handlers(void)
{
    signal(SIGINT, &graceful_stop);
    signal(SIGTERM, &graceful_stop);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: %s <max attempts> <cycles>\n", argv[0]);
        return -1;
    }

    size_t max_attempts = 0;
    if ((max_attempts = atoi(argv[1])) < 0)
    {
        printf("%s Using default value for attempts (%i)\n", WARNING, MAX_ATTEMPTS);
        max_attempts = MAX_ATTEMPTS;
    }

    size_t cycles = 0;
    if ((cycles = atoi(argv[2])) < 0)
    {
        printf("%s Using default value for cycles (%i)\n", WARNING, MAX_CYCLES);
        cycles = MAX_CYCLES;
    }

    game_role_t role = &game_run_server;
    pid_t ppid = getpid();

    sigset_init();

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
    if (semaphore_init(SEMAPHORE_NAME, cpid) == -1)
    {
        perror("semaphore_init");
        graceful_stop(0);
    }

    for (size_t i = 0; i < cycles; ++i)
    {
#ifdef DEBUG
        printf("%s cycle: %lu\n", cpid ? "parent" : "child", i);
#endif
        (*role)(ppid, cpid, max_attempts);
        role = game_role_swap(role, &ppid, &cpid);
    }
}
