#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "status.h"
#include "game/game.h"

#define BUFFERSIZE 4096
#define MIN_RANGE 3
#define MAX_RANGE (20 - MIN_RANGE + 1)

static void game_init(guess_number_t *game)
{
    srand(time(NULL));
    int range = MIN_RANGE + rand() % MAX_RANGE;
    game->range.bottom = rand();
    game->range.top = game->range.bottom + range;
    game->answer = game->range.bottom + rand() % (game->range.top - game->range.bottom + 1);
}

#ifdef PIPE2_MESSAGING
int game_run_server(int pipe_serverfd, int pipe_clientfd, size_t max_tries)
#else
int game_run_server(size_t max_tries)
#endif
{
    guess_number_t game;
    net_message_t message;
    int status = me_success;

    game_init(&game);
    game.tries = max_tries;
    printf("%s Game initiated with range [%i; %i] and answer=%i\n",
           INFO, game.range.bottom, game.range.top, game.answer);

#ifdef PIPE2_MESSAGING
    if (game_send_client_settings(
        pipe_clientfd, (game_client_settings_t *)&game) != me_success)
#else
    if ()
#endif
    {
        fprintf(stderr, "%s %s:Sending range:\t%s\n", strerror(errno));
        return -1;
    }

    do
    {
        printf("%s Client try #%i\n", INFO, max_tries - game.tries);
#ifdef PIPE2_MESSAGING
        if (game_receive_guess(client->_socket_descriptor, &game.guess) != me_success)
#else
        if ()
#endif
        {
            fprintf(stderr, "%s Receiving client guess:\t%s\n",
                    WARNING, strerror(errno));
            break;
        }

        printf("%s Client guess is %i\n", INFO, game.guess);

        int answer = a_right;
        if (game.guess - game.answer > 0)
            answer = a_more;
        else
            answer = game.guess - game.answer ? a_less : a_right;
            
#ifdef PIPE2_MESSAGING
        if (game_send_answer(client->_socket_descriptor, answer) != me_success)
#else
        if ()
#endif
        {
            fprintf(stderr, "%s Sending answer:\t%s\n", ERROR, strerror(errno));
            break;
        }
    } while (--game.tries);

    if (!game.tries)
        printf("%s Client exceeded amount of tries\n", INFO);

    return 0;
}
