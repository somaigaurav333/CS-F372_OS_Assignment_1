#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>

#define BUF_SIZE 200
#define PERMS 0644

struct message
{
    int client_id;
    int choice;
    char text[BUF_SIZE];
};

struct msg_buf
{
    long mtype;
    struct message msg;
};

int main(void)
{
    key_t key = 1;
    int msg_q_id;

    if ((key = ftok("server.c", 1)) == -1)
    {
        perror("ftok Failed\n");
        exit(1);
    }

    printf("ftok key: %d\n", key);

    if ((msg_q_id = msgget(key, PERMS)) == -1)
    {
        perror("Failed to connect to Message Queue\n");
        exit(1);
    }

    printf("msg_q_id: %d\n", msg_q_id);

    struct msg_buf msg_snd;
    msg_snd.mtype = 1;
    msg_snd.msg.choice = 0;
    msg_snd.msg.client_id = 0;

    int exit = 0;
    char choice = 'N';

    while (exit == 0)
    {
        printf("Do you want the server to terminate? Press Y for Yes and N for No: ");
        scanf(" %c", &choice);

        if (choice == 'Y')
        {
            msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0);
            exit = 1;
            break;
        }
    }

    return 0;
}