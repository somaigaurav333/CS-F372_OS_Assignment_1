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
#include <stdbool.h>

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

struct pipe_msg
{
    int client_id;
    int choice;
    char ptext[BUF_SIZE];
};

int cleanup(int msg_q_id, int fdr, int fdw)
{
    msgctl(msg_q_id, IPC_RMID, NULL);
    close(fdr);
    close(fdw);

    return 0;
}

bool exists(const char *filename)
{
    return access(filename, F_OK) == 0;
}

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

    if ((msg_q_id = msgget(key, PERMS | IPC_CREAT)) == -1)
    {
        perror("Failed to connect to Message Queue\n");
        exit(1);
    }

    printf("msg_q_id: %d\n", msg_q_id);

    struct msg_buf msg_rcv;
    pid_t pid = 1;

    int fd[2];
    if (pipe(fd) == -1)
    {
        fprintf(stderr, "Pipe Failed");
        msgctl(msg_q_id, IPC_RMID, NULL);
        exit(1);
    }

    while (pid > 0)
    {
        if (msgrcv(msg_q_id, &msg_rcv, sizeof(msg_rcv), 1, 0) == -1)
        {
            perror("msgrcv error ");
            printf("%d\n", errno);
            break;
        }

        if (msg_rcv.msg.client_id == 0)
        {
            while (wait(NULL) > 0)
                ;
            break;
        }

        printf("Client %d : %s \n", msg_rcv.msg.client_id, msg_rcv.msg.text);

        if ((pid = fork()) < 0)
        {
            perror("Fork failed");
            cleanup(msg_q_id, fd[0], fd[1]);
            exit(1);
        }

        if (pid > 0)
        {

            struct pipe_msg pipe_msg_snd;
            pipe_msg_snd.client_id = msg_rcv.msg.client_id;
            pipe_msg_snd.choice = msg_rcv.msg.choice;
            strncpy(pipe_msg_snd.ptext, msg_rcv.msg.text, BUF_SIZE);

            if (write(fd[1], &(pipe_msg_snd), sizeof(pipe_msg_snd)) == -1)
            {
                perror("Pipe write");
                cleanup(msg_q_id, fd[0], fd[1]);
                exit(1);
            }
        }
        else if (pid == 0)
        {

            struct pipe_msg pipe_msg_rcv;
            if (read(fd[0], &pipe_msg_rcv, sizeof(pipe_msg_rcv)) == -1)
            {
                perror("Pipe read");
                exit(1);
            }
            int choice = pipe_msg_rcv.choice, client_id = pipe_msg_rcv.client_id;
            printf("client_id: %d   choice: %d\n", client_id, choice);

            struct msg_buf msg_snd;
            msg_snd.mtype = client_id + 2;
            msg_snd.msg.client_id = client_id;

            if (choice == 1)
            {
                char hello[BUF_SIZE] = "Hello";
                msg_snd.msg.choice = choice;
                strncpy(msg_snd.msg.text, hello, BUF_SIZE);
                if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                {
                    perror("msgsnd");
                    printf("%d\n", errno);
                    exit(1);
                }
            }
            else if (choice == 2)
            {

                int pid2;
                int fd2[2];
                char filename[BUF_SIZE];
                strncpy(filename, pipe_msg_rcv.ptext, BUF_SIZE);

                // check for existence of file
                if (!exists(filename))
                {
                    printf("File doesn't exist\n");
                    char msg[BUF_SIZE] = "File not found";
                    strncpy(msg_snd.msg.text, msg, BUF_SIZE);
                    if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                    {
                        perror("msgsnd");
                        printf("%d\n", errno);
                        exit(1);
                    }
                }
                else
                {
                    printf("File exists\n");
                    char msg[BUF_SIZE] = "File exists";
                    strncpy(msg_snd.msg.text, msg, BUF_SIZE);
                    if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                    {
                        perror("msgsnd");
                        printf("%d\n", errno);
                        exit(1);
                    }
                }
            }
            else if (choice == 3)
            {

                int pid3;
                int fd3[2];
                char filename[BUF_SIZE];
                strncpy(filename, pipe_msg_rcv.ptext, BUF_SIZE);

                // check for existence of file
                if (!exists(filename))
                {
                    printf("File doesn't exist\n");
                    char msg[BUF_SIZE] = "File not found";
                    strncpy(msg_snd.msg.text, msg, BUF_SIZE);
                    if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                    {
                        perror("msgsnd");
                        printf("%d\n", errno);
                        exit(1);
                    }
                }
                else
                {

                    if (pipe(fd3) == -1)
                    {
                        perror("pipe creation failed");
                    }

                    pid3 = fork();
                    char word_string[BUF_SIZE]; // Buffer to store the output from "wc"

                    if (pid3 == 0)
                    {
                        // Close the read end of the pipe in the child process
                        close(fd3[0]);

                        // Redirect stdout to the pipe
                        dup2(fd3[1], STDOUT_FILENO);
                        if (execlp("wc", "wc", "-w", filename, NULL) == -1)
                        {
                            perror("File does not exist");
                            exit(1);
                        }
                    }
                    else
                    {
                        wait(NULL);

                        // Close the write end of the pipe in the parent process
                        close(fd3[1]);

                        ssize_t word_count = read(fd3[0], word_string, BUF_SIZE);

                        if (word_count < 0)
                        {
                            perror("Failed to read from pipe");
                            exit(1);
                        }

                        bool end = 0;
                        for (int i = 0; i < BUF_SIZE; i++)
                        {
                            if (word_string[i] == ' ')
                            {
                                end = 1;
                            }

                            if (end == 1)
                            {
                                word_string[i] = '\0';
                            }
                        }
                    }

                    strncpy(msg_snd.msg.text, word_string, BUF_SIZE);
                    if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                    {
                        perror("msgsnd");
                        printf("%d\n", errno);
                        exit(1);
                    }
                }
            }

            exit(0);
        }
    }

    cleanup(msg_q_id, fd[0], fd[1]);
    printf("Exit successful\n");

    return 0;
}