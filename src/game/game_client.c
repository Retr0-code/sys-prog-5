#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "status.h"
#include "game/game.h"

static range_t last_guess = {0, 0};

void guess_function(const range_t *range, int *guess, int answer)
{
    if (last_guess.bottom == 0 && last_guess.top == 0)
    {
        last_guess.top = range->top;
        last_guess.bottom = range->bottom;
        *guess = last_guess.top;
        return ;
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

int game_run_client(int serverfd, int clientfd, size_t max_tries)
{
    game_client_settings_t game_settings;
    int guess = 0;
    int status = me_success;
    int answer = a_right;
    while (1)
    {
        status = game_receive_client_settings(clientfd, &game_settings);
        switch (status)
        {
        case me_receive:
            fprintf(stderr, "%s Something went wrong while receiving:\t%s\n", ERROR, strerror(errno));
            break;
        case me_peer_end:
            fprintf(stderr, "%s Server closed connection\n", ERROR);
            break;
        default:
            break;
        }
        if (status != me_success)
            break;
        do
        {
            printf("#%li tries left.\nGuess the number in range [%i; %i]> \n",
                   game_settings.tries, game_settings.range.bottom, game_settings.range.top);
            guess_function(&game_settings.range, &guess, answer);
            if (game_send_guess(serverfd, guess) != me_success)
            {
                fprintf(stderr, "%s Something went wrong while sending:\t%s\n", ERROR, strerror(errno));
                break;
            }

            if (game_receive_answer(clientfd, &answer) != me_success)
            {
                fprintf(stderr, "%s Something went wrong while receiving answer:\t%s\n", ERROR, strerror(errno));
                break;
            }

            if (a_right == answer)
            {
                printf("%i is right answer\n", guess);
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

    last_guess.bottom = 0;
    last_guess.top = 0;
    return 0;
}
