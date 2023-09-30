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
    int client_id;
    key_t key = 1;
    int msg_q_id;

    printf("Enter Client ID: ");
    scanf("%d", &client_id);

    // Generate key using ftok
    if ((key = ftok("server.c", 1)) == -1)
    {
        perror("ftok error");
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
    msg_snd.msg.client_id = client_id;
    msg_snd.mtype = 1;

    while (1)
    {
        // Take choice input from user

        int choice;
        printf("\nEnter 1 to contact the Ping Server\n");
        printf("Enter 2 to contact the File Search Server\n");
        printf("Enter 3 to contact the File Word Count Server\n");
        printf("Enter 4 if this Client wishes to exit\n");
        printf("Enter choice:   ");
        scanf("%d", &choice);
        printf("\n");

        msg_snd.msg.choice = choice;

        int msg_rcv_id = client_id + 2;

        if (choice == 1) // Client choice is 1
        {
            // Send message "Hi" to server
            char hi[BUF_SIZE] = "Hi";
            strncpy(msg_snd.msg.text, hi, BUF_SIZE);

            if (msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0) == -1)
            {
                perror("Failed to send message");
                exit(EXIT_FAILURE);
            }

            // Receive message from server
            struct msg_buf msg_rcv;

            if ((msgrcv(msg_q_id, &msg_rcv, sizeof(msg_rcv), msg_rcv_id, 0)) == -1)
            {
                perror("Failed to receive message");
                printf("%d\n", errno);
                exit(EXIT_FAILURE);
            }

            printf("Server: %s\n", msg_rcv.msg.text);
        }
        else if (choice == 2) // Client choice is 2
        {
            // Take filename as input
            printf("Enter file name: ");
            char filename[BUF_SIZE];
            scanf("%s", filename);
            printf("\n");

            // Send message to server
            strncpy(msg_snd.msg.text, filename, BUF_SIZE);
            if (msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0) == -1)
            {
                perror("Failed to send message");
                exit(EXIT_FAILURE);
            }

            // Receive message from server
            struct msg_buf msg_rcv;

            if ((msgrcv(msg_q_id, &msg_rcv, sizeof(msg_rcv), msg_rcv_id, 0)) == -1)
            {
                perror("Failed to receive message");
                printf("%d\n", errno);
                exit(EXIT_FAILURE);
            }

            printf("Server: %s\n", msg_rcv.msg.text);
        }
        else if (choice == 3) // Client choice is 3
        {
            // Take filename as input
            printf("Enter file name: ");
            char filename[BUF_SIZE];
            scanf("%s", filename);
            printf("\n");

            // Send message to server
            strncpy(msg_snd.msg.text, filename, BUF_SIZE);
            if (msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0) == -1)
            {
                perror("Failed to send message");
                exit(EXIT_FAILURE);
            }

            // Receive message from server
            struct msg_buf msg_rcv;

            if ((msgrcv(msg_q_id, &msg_rcv, sizeof(msg_rcv), msg_rcv_id, 0)) == -1)
            {
                perror("Failed to receive message");
                printf("%d\n", errno);
                exit(EXIT_FAILURE);
            }

            printf("Server: %s\n", msg_rcv.msg.text);
        }
        else if (choice == 4) // Client choice is 4
        {
            // Exit
            break;
        }
        else // Invalid Client Choice
        {
            printf("Please enter a valid choice\n");
            continue;
        }
    }

    // Graceful Termination
    printf("Exit successful\n");

    return 0;
}
