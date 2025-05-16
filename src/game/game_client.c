#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "status.h"
#include "game/game.h"

static range_t last_guess = {0, 0};

static void guess_function(const range_t *range, int *guess, int answer)
{
    if (last_guess.bottom == 0 && last_guess.top == 0)
    {
        last_guess.top = range->top;
        last_guess.bottom = range->bottom;
        *guess = last_guess.top;
        return;
    }

    switch (answer)
    {
    case a_less:
        last_guess.top = last_guess.bottom + (last_guess.top - last_guess.bottom) / 2;
        *guess = last_guess.top;
        break;

    case a_more:
        last_guess.bottom = (*guess - last_guess.bottom) / 2 + last_guess.bottom;
        *guess = last_guess.bottom;
        break;

    default:
        break;
    }
}

static void print_receiver_errors(int status)
{
    switch (status)
    {
    case me_receive:
        fprintf(stderr, "%s Something went wrong while receiving:\t%s\n", ERROR, strerror(errno));
        break;
    case me_peer_end:
        fprintf(stderr, "%s Server closed connection\n", ERROR);
        break;
    case me_bad_fd:
        fprintf(stderr, "%s Receiver bad file descriptor\n", ERROR);
        break;
    case me_sync:
        fprintf(stderr, "%s Receiver synchronization error:\t%s\n", ERROR, strerror(errno));
        break;
    case me_invalid_args:
        fprintf(stderr, "%s Receiver invalid arguments\n", ERROR);
        break;
    case me_wrong_type:
        fprintf(stderr, "%s Receiver got an unexpected message\n", ERROR);
        break;
    case me_wrong_length:
        fprintf(stderr, "%s Receiver got broken payload\n", ERROR);
        break;
    default:
        fprintf(stderr, "%s Receiver unknown error %i\n", ERROR, status);
        break;
    }
}

int game_run_client(int serverfd, int clientfd, size_t max_tries, game_stats_t *stats)
{
    game_client_settings_t game_settings;
    int guess = 0;
    int status = me_success;
    int answer = a_right;
    while (1)
    {
        status = game_receive_client_settings(clientfd, &game_settings);
        if (status != me_success)
        {
            print_receiver_errors(status);
            return 1;
        }

        do
        {
            printf("#%li tries left.\nGuess the number in range [%i; %i]> \n",
                   game_settings.tries, game_settings.range.bottom, game_settings.range.top);
            guess_function(&game_settings.range, &guess, answer);
            if (game_send_guess(serverfd, guess) != me_success)
            {
                fprintf(stderr, "%s Something went wrong while sending:\t%s\n", ERROR, strerror(errno));
                return 2;
            }

            if ((status = game_receive_answer(clientfd, &answer)) != me_success)
            {
                fprintf(stderr, "%s Something went wrong while receiving answer\n", ERROR);
                print_receiver_errors(status);
                return 3;
            }

            if (a_right == answer)
            {
                printf("%i is right answer\n", guess);
                stats->client = 1;
                stats->server = 0;
                stats->tries = max_tries - game_settings.tries + 1;
                break;
            }

            switch (answer)
            {
            case a_less:
                printf("%i is less than answer\n", guess);
                break;

            case a_more:
                printf("%i is more than answer\n", guess);
                break;

            default:
                break;
            }
        } while (--game_settings.tries);
        break;
    }
    if (a_right != answer)
    {
        stats->client = 0;
        stats->server = 1;
        stats->tries = max_tries - game_settings.tries;
    }

    last_guess.bottom = 0;
    last_guess.top = 0;
    return 0;
}
