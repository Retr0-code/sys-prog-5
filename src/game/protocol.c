#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

#include "game/protocol.h"

#ifdef PIPE2_MESSAGING
int message_send(int fd, int type, size_t data_length, const char *data)
{
    if (data_length == 0 && data != NULL || data_length != 0 && data == NULL)
    {
        errno = EINVAL;
        return me_invalid_args;
    }

    if (fd <= 0)
    {
        errno = EBADF;
        return me_bad_fd;
    }

    net_message_t message = { type, data_length, data };
    if (send(fd, &message, sizeof(message) - sizeof(message.body), MSG_MORE) == -1)
        return me_send;

    if (data_length == 0)
        return me_success;

    if (send(fd, message.body, message.length, 0) == -1)
        return me_send;

    return me_success;
}

int message_receive(int fd, int type, size_t data_length, char **data)
{
    if (fd <= 0)
    {
        errno = EBADF;
        return me_bad_fd;
    }

    net_message_t message = { 0, 0, NULL };
    ssize_t status = 0;
    if ((status = recv(fd, &message, sizeof(message) - sizeof(message.body), 0)) == -1)
        return me_receive;

    if (status == 0)
        return me_peer_end;

    if (type != message.type)
        return me_wrong_type;

    if (data_length != message.length && data_length)
        return me_wrong_length;

    if (message.length == 0)
        return me_success;

    message.body = data_length ? *data : malloc(message.length);
    if (message.body == NULL)
        return me_receive;

    if ((status = recv(fd, message.body, message.length, 0)) == -1)
    {
        if (data_length == 0)
            free(message.body);

        return me_receive;
    }

    if (status == 0)
        return me_peer_end;

    *data = message.body;
    return me_success;
}
#else
int message_send(int fd, int type, size_t data_length, const char *data)
{
    if (data_length == 0 && data != NULL || data_length != 0 && data == NULL)
    {
        errno = EINVAL;
        return me_invalid_args;
    }

    if (fd <= 0)
    {
        errno = EBADF;
        return me_bad_fd;
    }

    net_message_t message = { type, data_length, data };
    if (sigqueue(fd, SIGUSR1, &message) == -1)
        return me_send;

    if (data_length == 0)
        return me_success;

    if (send(fd, message.body, message.length, 0) == -1)
        return me_send;

    return me_success;
}

int message_receive(int fd, int type, size_t data_length, char **data)
{
    if (fd <= 0)
    {
        errno = EBADF;
        return me_bad_fd;
    }

    net_message_t message = { 0, 0, NULL };
    ssize_t status = 0;
    if ((status = recv(fd, &message, sizeof(message) - sizeof(message.body), 0)) == -1)
        return me_receive;

    if (status == 0)
        return me_peer_end;

    if (type != message.type)
        return me_wrong_type;

    if (data_length != message.length && data_length)
        return me_wrong_length;

    if (message.length == 0)
        return me_success;

    message.body = data_length ? *data : malloc(message.length);
    if (message.body == NULL)
        return me_receive;

    if ((status = recv(fd, message.body, message.length, 0)) == -1)
    {
        if (data_length == 0)
            free(message.body);

        return me_receive;
    }

    if (status == 0)
        return me_peer_end;

    *data = message.body;
    return me_success;
}
#endif