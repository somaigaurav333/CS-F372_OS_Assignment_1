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

#define BUF_SIZE 300
#define PERMS 0644

struct message
{
    int client_id;
    int choice;
    char text[BUF_SIZE];
};

/*
    Message Queue Buffer

    Client  ->  Server - mtype = 1
    Cleanup ->  Server - mtype = 1, client_id = 0, choice = 0
    Server  ->  Client - mtype = 2 + client_id

*/
struct msg_buf
{
    long mtype;
    struct message msg;
};

int main(void)
{
    key_t key = 1;
    int msg_q_id;

    // Generate key using ftok
    if ((key = ftok("server.c", 1)) == -1)
    {
        perror("ftok Failed\n");
        exit(EXIT_FAILURE);
    }

    // Connect to the message queue for IPC
    if ((msg_q_id = msgget(key, PERMS)) == -1)
    {
        perror("Failed to connect to Message Queue\n");
        exit(EXIT_FAILURE);
    }

    // Struct Variable to send the message into message queue
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

        if (choice == 'Y') // User enters 'Y'
        {
            // Send message to server to quit
            msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0);
            exit = 1;
            break;
        }
    }

    return 0;
}