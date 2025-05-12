#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>

#include "game/protocol.h"

static sem_t *semaphore = NULL;
static sem_t *semaphore_pipe = NULL;
static int semaphore_closed = 0;
static int semaphore_pipe_closed = 0;

#define SEMAPHORE_PIPE_POSTFIX "_pipe"

int semaphore_init(const char *file, pid_t cpid)
{
#ifdef PIPE2_MESSAGING
    char semaphore_pipe_name[FILENAME_MAX];
    strncpy(semaphore_pipe_name, file, FILENAME_MAX);
    strncat(semaphore_pipe_name, SEMAPHORE_PIPE_POSTFIX, FILENAME_MAX);
#endif

    if (cpid == 0)
    {
        if ((semaphore = sem_open(file, O_CREAT | O_RDWR)) == SEM_FAILED)
            return -1;

#ifdef PIPE2_MESSAGING
        if ((semaphore_pipe = sem_open(semaphore_pipe_name, O_CREAT | O_RDWR)) == SEM_FAILED)
            return -1;
#endif

        return 0;
    }

    if ((semaphore = sem_open(file, O_CREAT | O_RDWR, 0644, 0)) == SEM_FAILED)
        return -1;

    if (sem_init(semaphore, 1, 0) == -1)
        return -1;

#ifdef PIPE2_MESSAGING
    if ((semaphore_pipe = sem_open(semaphore_pipe_name, O_CREAT | O_RDWR, 0644, 0)) == SEM_FAILED)
        return -1;

    if (sem_init(semaphore_pipe, 1, 0) == -1)
        return -1;
#endif
    return 0;
}

int semaphore_close(const char *file)
{
    if (semaphore_closed != 0)
    {
        sem_unlink(file);
        sem_close(semaphore);
        sem_destroy(semaphore);
        semaphore_closed = 1;
    }
#ifdef PIPE2_MESSAGING
    if (semaphore_pipe_closed != 0)
    {
        char semaphore_pipe_name[FILENAME_MAX];
        strncpy(semaphore_pipe_name, file, FILENAME_MAX);
        strncat(semaphore_pipe_name, SEMAPHORE_PIPE_POSTFIX, FILENAME_MAX);
        
        sem_unlink(semaphore_pipe_name);
        sem_close(semaphore_pipe);
        sem_destroy(semaphore_pipe);
        semaphore_pipe_closed = 1;
    }
#endif

    return 0;
}

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

    net_message_t message = {type, data_length, data};
#ifdef DEBUG
    printf("message_send: type: %i, length: %lu, data: %p\n", message.type, message.length, message.body);
#endif

    if (write(fd, &message, sizeof(message) - sizeof(message.body)) == -1)
        return me_send;

    if (data_length == 0)
        return me_success;

    if (write(fd, message.body, message.length) == -1)
        return me_send;

    if (sem_post(semaphore) == -1)
        return me_sync;

#ifdef DEBUG
    int sem_value = 0;
    sem_getvalue(semaphore, &sem_value);
    printf("send sem_unlocked: %i\n", sem_value);
#endif

    if (sem_wait(semaphore_pipe) == -1)
        return me_sync;

#ifdef DEBUG
    sem_getvalue(semaphore_pipe, &sem_value);
    printf("send sem_pipe_unlocked: %i\n", sem_value);
#endif

    return me_success;
}

int message_receive(int fd, int type, size_t data_length, char **data)
{
    if (fd <= 0)
    {
        errno = EBADF;
        return me_bad_fd;
    }

    ssize_t status = 0;
    net_message_t message = {0, 0, NULL};

#ifdef DEBUG
    int sem_value = 0;
    sem_getvalue(semaphore, &sem_value);
    printf("receive sem_locked: %i\n", sem_value);
#endif

    if (sem_wait(semaphore) == -1)
        return me_sync;

#ifdef DEBUG
    sem_getvalue(semaphore, &sem_value);
    printf("receive sem_unlocked: %i\n", sem_value);
#endif

    if ((status = read(fd, &message, sizeof(message) - sizeof(message.body))) == -1)
        return me_receive;

    if (status == 0)
        return me_peer_end;

#ifdef DEBUG
    printf("message_receive: type: %i, length: %lu\n", message.type, message.length);
#endif

    if (type != message.type)
        return me_wrong_type;

    if (data_length != message.length && data_length)
        return me_wrong_length;

    if (message.length == 0)
        return me_success;

    message.body = data_length ? *data : malloc(message.length);
    if (message.body == NULL)
        return me_receive;

    if ((status = read(fd, message.body, message.length)) == -1)
    {
        if (data_length == 0)
            free(message.body);

        return me_receive;
    }

#ifdef DEBUG
    printf("message_receive: data: %p\n", message.body);
#endif

    if (status == 0)
    {
        if (data_length == 0)
            free(message.body);

        return me_peer_end;
    }

    if (sem_post(semaphore_pipe) == -1)
    {
        if (data_length == 0)
            free(message.body);

        return me_sync;
    }
#ifdef DEBUG
    sem_getvalue(semaphore_pipe, &sem_value);
    printf("receive sem_pipe_unlocked: %i\n", sem_value);
#endif

    *data = message.body;
    return me_success;
}

#else
static sigset_t *sigset_operational = NULL;

int sigset_init(void)
{
    sigset_operational = malloc(sizeof(sigset_t));
    if (sigemptyset(sigset_operational) == -1)
        return -1;

    sigaddset(sigset_operational, SIGUSR1);
    sigaddset(sigset_operational, SIGUSR2);
    return 0;
}

typedef struct
{
    const sigset_t *set;
    siginfo_t *info;
    int err;
} sigwaitinfo_args_t;

int thrd_sigwaitinfo(sigwaitinfo_args_t *args)
{
    int status = sigwaitinfo(args->set, args->info);
    args->err = errno;
    return status;
}

int message_send(int pid, int type, size_t data_length, const char *data)
{
    if ((data_length == 0 && data != NULL) || (data_length != 0 && data == NULL))
    {
        errno = EINVAL;
        return me_invalid_args;
    }

    if (pid < 0)
    {
        errno = EBADF;
        return me_bad_fd;
    }

    if (data_length % sizeof(int))
        data_length += sizeof(int) - (data_length % sizeof(int));

#ifdef DEBUG
    int sem_value = 0;
    sem_getvalue(semaphore, &sem_value);
    printf("send sem_locked: %i\n", sem_value);
#endif
    if (sem_wait(semaphore) == -1)
        return me_sync;

#ifdef DEBUG
    sem_getvalue(semaphore, &sem_value);
    printf("send sem_unlocked: %i\n", sem_value);
#endif

    net_message_t message = {type, data_length, data};
    if (sigqueue(pid, SIGUSR1, (sigval_t)(int)(message.type)) == -1)
        return me_send;
#ifdef DEBUG
    printf("sigqueue \"type\" %i\n", message.type);
#endif

    if (sem_wait(semaphore) == -1)
        return me_sync;

    if (sigqueue(pid, SIGUSR1, (sigval_t)(int)(message.length)) == -1)
        return me_send;

#ifdef DEBUG
    printf("sigqueue \"length\" %i\n", message.length);
#endif

    if (data_length == 0)
        return me_success;

    int *i32data = (int *)message.body;
    for (size_t i = 0; i < data_length / sizeof(int); ++i)
    {
        if (sem_wait(semaphore) == -1)
            return me_sync;

        if (sigqueue(pid, SIGUSR1, (sigval_t)(i32data[i])) == -1)
            return me_send;
#ifdef DEBUG
        printf("sigqueue \"chunk%i\" %i\n", i, i32data[i]);
#endif
    }

    return me_success;
}

int message_receive(int fd, int type, size_t data_length, char **data)
{
    if (data == NULL)
    {
        errno = EINVAL;
        return me_invalid_args;
    }

    if (fd < 0)
    {
        errno = EBADF;
        return me_bad_fd;
    }

    pthread_t sigwait_thread;
    int sigwait_status;
    siginfo_t siginfo;
    memset(&siginfo, 0, sizeof(siginfo_t));
    net_message_t message = {0, 0, NULL};

    int pthread_error = 0;
    if ((pthread_error = pthread_sigmask(SIG_BLOCK, sigset_operational, NULL)) != 0)
    {
        errno = pthread_error;
        return me_receive;
    }

    sigwaitinfo_args_t args = {sigset_operational, &siginfo, 0};
    if (pthread_create(&sigwait_thread, NULL, &thrd_sigwaitinfo, &args) != 0)
        return me_receive;

#ifdef DEBUG
    int sem_value = 0;
    sem_getvalue(semaphore, &sem_value);
    printf("recv sem_locked: %i\n", sem_value);
#endif
    if (sem_post(semaphore) == -1)
        return me_sync;

#ifdef DEBUG
    sem_getvalue(semaphore, &sem_value);
    printf("recv sem_unlocked: %i\n", sem_value);
#endif

    if (pthread_join(sigwait_thread, &sigwait_status) != 0 || sigwait_status == -1)
    {
        errno = args.err;
        return me_receive;
    }

    message.type = siginfo.si_value.sival_int;
#ifdef DEBUG
    printf("sigwaitinfo \"type\" %i\n", message.type);
#endif

    if (pthread_create(&sigwait_thread, NULL, &thrd_sigwaitinfo, &args) != 0)
        return me_receive;

    if (sem_post(semaphore) == -1)
        return me_sync;

    if (pthread_join(sigwait_thread, &sigwait_status) != 0 || sigwait_status == -1)
    {
        errno = args.err;
        return me_receive;
    }

    message.length = siginfo.si_value.sival_int;

#ifdef DEBUG
    printf("sigwaitinfo \"length\" %i\n", message.length);
#endif

    message.body = data_length ? *data : malloc(message.length);
    if (message.body == NULL)
        return me_receive;

    int *i32data = (int *)message.body;
    for (size_t i = 0; i < message.length / sizeof(int); ++i)
    {
        if (pthread_create(&sigwait_thread, NULL, &thrd_sigwaitinfo, &args) != 0)
            return me_receive;

        if (sem_post(semaphore) == -1)
            return me_sync;

        if (pthread_join(sigwait_thread, &sigwait_status) != 0 || sigwait_status == -1)
        {
            if (data_length == 0)
                free(message.body);

            errno = args.err;
            return me_receive;
        }

        i32data[i] = siginfo.si_value.sival_int;
#ifdef DEBUG
        printf("sigwaitinfo \"chunk%i\" %i\n", i, i32data[i]);
#endif
    }

    *data = message.body;
    return me_success;
}
#endif
