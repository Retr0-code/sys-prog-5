#include "game/game.h"
#include "game/protocol.h"

int game_send_client_settings(int client_fd, const game_client_settings_t *settings)
{
    return message_send(client_fd, mt_range, sizeof(game_client_settings_t), settings);
}

int game_receive_client_settings(int client_fd, game_client_settings_t *settings)
{
    return message_receive(client_fd, mt_range, sizeof(game_client_settings_t), &settings);
}

int game_send_guess(int client_fd, int guess)
{
    return message_send(client_fd, mt_guess, sizeof(int), &guess);
}

int game_receive_guess(int client_fd, int *guess)
{
    return message_receive(client_fd, mt_guess, sizeof(int), &guess);
}

int game_send_answer(int client_fd, int answer)
{
    return message_send(client_fd, mt_answer, sizeof(int), &answer);
}

int game_receive_answer(int client_fd, int *answer)
{
    return message_receive(client_fd, mt_answer, sizeof(int), &answer);
}
