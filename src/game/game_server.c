#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "status.h"
#include "game/game.h"

#define BUFFERSIZE 4096
#define MIN_RANGE 3
#define MAX_RANGE (20 - MIN_RANGE + 1)

static void game_init(guess_number_t *game)
{
    uint32_t seed = 0;
    FILE *dev_random = fopen("/dev/random", "r");
    if (dev_random)
    {
        if (fread(&seed, sizeof(seed), 1, dev_random) != 1)
            seed = time(NULL);

        fclose(dev_random);
    }
    else
        seed = time(NULL);

    srand(seed);
    int range = MIN_RANGE + rand() % MAX_RANGE;
    game->range.bottom = rand();
    game->range.top = game->range.bottom + range;
    game->answer = game->range.bottom + rand() % (game->range.top - game->range.bottom + 1);
}

int game_run_server(int serverfd, int clientfd, size_t max_tries)
{
    guess_number_t game;

    game_init(&game);
    game.tries = max_tries;
    printf("%s Game initiated with range [%i; %i] and answer=%i\n",
           INFO, game.range.bottom, game.range.top, game.answer);

    if (game_send_client_settings(clientfd, (game_client_settings_t *)&game) != me_success)
    {
        fprintf(stderr, "%s Sending range:\t%s\n", ERROR, strerror(errno));
        return 1;
    }

    do
    {
        printf("%s Client try #%li\n", INFO, max_tries - game.tries + 1);
        if (game_receive_guess(serverfd, &game.guess) != me_success)
        {
            fprintf(stderr, "%s Receiving client guess:\t%s\n",
                    WARNING, strerror(errno));
            return 2;
            // break;
        }

        printf("%s Client guess is %i\n", INFO, game.guess);

        int answer = a_right;
        if (game.guess - game.answer > 0)
            answer = a_more;
        else
            answer = game.guess - game.answer ? a_less : a_right;

        if (game_send_answer(clientfd, answer) != me_success)
        {
            fprintf(stderr, "%s Sending answer:\t%s\n", ERROR, strerror(errno));
            return 3;
            // break;
        }

        if (answer == a_right)
            break;
    } while (--game.tries);

    if (!game.tries)
        printf("%s Client exceeded amount of tries\n", INFO);

    return 0;
}
