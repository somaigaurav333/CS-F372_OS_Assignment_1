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

// Message Queue Buffer
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

    if ((key = ftok("server.c", 1)) == -1)
    {
        perror("ftok error");
        exit(EXIT_FAILURE);
    }

    if ((msg_q_id = msgget(key, PERMS)) == -1)
    {
        perror("Failed to connect to Message Queue\n");
        exit(EXIT_FAILURE);
    }

    /*
    UNCOMMENT FOR DEBUGGING
    printf("ftok key: %d\n", key);
    printf("msg_q_id: %d\n", msg_q_id);
    */

    struct msg_buf msg_snd;
    msg_snd.msg.client_id = client_id;
    msg_snd.mtype = 1;

    while (1)
    {
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

        if (choice == 1)
        {
            char hi[BUF_SIZE] = "Hi";
            strncpy(msg_snd.msg.text, hi, BUF_SIZE);
            if (msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0) == -1)
            {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }

            struct msg_buf msg_rcv;

            if ((msgrcv(msg_q_id, &msg_rcv, sizeof(msg_rcv), msg_rcv_id, 0)) == -1)
            {
                perror("msgrcv error ");
                printf("%d\n", errno);
                exit(EXIT_FAILURE);
            }

            printf("Server: %s\n", msg_rcv.msg.text);
        }
        else if (choice == 2)
        {
            printf("Enter file name: ");
            char filename[BUF_SIZE];
            scanf("%s", filename);
            printf("\n");

            strncpy(msg_snd.msg.text, filename, BUF_SIZE);
            if (msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0) == -1)
            {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }

            struct msg_buf msg_rcv;

            if ((msgrcv(msg_q_id, &msg_rcv, sizeof(msg_rcv), msg_rcv_id, 0)) == -1)
            {
                perror("msgrcv error ");
                printf("%d\n", errno);
                exit(EXIT_FAILURE);
            }

            printf("Server: %s\n", msg_rcv.msg.text);
        }
        else if (choice == 3)
        {
            printf("Enter file name: ");
            char filename[BUF_SIZE];
            scanf("%s", filename);
            printf("\n");

            strncpy(msg_snd.msg.text, filename, BUF_SIZE);
            if (msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0) == -1)
            {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }

            struct msg_buf msg_rcv;

            if ((msgrcv(msg_q_id, &msg_rcv, sizeof(msg_rcv), msg_rcv_id, 0)) == -1)
            {
                perror("msgrcv error ");
                printf("%d\n", errno);
                exit(EXIT_FAILURE);
            }

            printf("Server: %s\n", msg_rcv.msg.text);
        }
        else if (choice == 4)
        {
            break;
        }
        else
        {
            printf("Please enter a valid choice\n");
            continue;
        }
    }

    printf("Exit successful\n");

    return 0;
}
