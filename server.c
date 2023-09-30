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

// Message Queue Buffer
struct msg_buf
{
    long mtype;
    struct message msg;
};

// Pipe Message Struct
struct pipe_msg
{
    int client_id;
    int choice;
    char ptext[BUF_SIZE];
};

// Function to cleanup message queue and pipes
void cleanup(int msg_q_id, int fdr, int fdw)
{
    msgctl(msg_q_id, IPC_RMID, NULL);
    close(fdr);
    close(fdw);
}

// Function to check if a specified file exists
bool exists(const char *filename)
{
    return access(filename, F_OK) == 0;
}

// Function to extract word count of file from execlp
void get_word_count(char *word_string)
{
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

    if ((msg_q_id = msgget(key, PERMS | IPC_CREAT)) == -1)
    {
        perror("Failed to connect to Message Queue\n");
        exit(EXIT_FAILURE);
    }

    /*
    UNCOMMENT FOR DEBUGGING
    printf("ftok key: %d\n", key);
    printf("msg_q_id: %d\n", msg_q_id);
    */

    // Struct variable to receive the message from Message Queue
    struct msg_buf msg_rcv;

    pid_t pid = 1;

    // Creating pipe for IPC between parent to child
    int fd[2];
    if (pipe(fd) == -1)
    {
        fprintf(stderr, "Pipe Failed");
        msgctl(msg_q_id, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    while (pid > 0)
    {

        // Read message from Message Queue intended for Server
        if (msgrcv(msg_q_id, &msg_rcv, sizeof(msg_rcv), 1, 0) == -1)
        {
            perror("Message could not be sent.");
            printf("%d\n", errno);
            break;
        }

        // Check if the message request is for Cleanup
        if (msg_rcv.msg.client_id == 0)
        {
            // Wait for all childs to terminate
            while (wait(NULL) > 0)
                ;
            break;
        }

        // Print the text of message received
        printf("Client %d : %s \n", msg_rcv.msg.client_id, msg_rcv.msg.text);

        // Create a child server to unload the task upon
        if ((pid = fork()) < 0)
        {
            perror("Fork failed\n");
            cleanup(msg_q_id, fd[0], fd[1]);
            exit(EXIT_FAILURE);
        }

        // Parent process
        if (pid > 0)
        {
            // Write the message received from client into pipe
            struct pipe_msg pipe_msg_snd;
            pipe_msg_snd.client_id = msg_rcv.msg.client_id;
            pipe_msg_snd.choice = msg_rcv.msg.choice;
            strncpy(pipe_msg_snd.ptext, msg_rcv.msg.text, BUF_SIZE);

            if (write(fd[1], &(pipe_msg_snd), sizeof(pipe_msg_snd)) == -1)
            {
                perror("Could not write to pipe");
                cleanup(msg_q_id, fd[0], fd[1]);
                exit(EXIT_FAILURE);
            }
        }
        else if (pid == 0) // Child process
        {

            // Read the message sent via parent from pipe
            struct pipe_msg pipe_msg_rcv;
            if (read(fd[0], &pipe_msg_rcv, sizeof(pipe_msg_rcv)) == -1)
            {
                perror("Pipe read failure");
                exit(EXIT_FAILURE);
            }

            int choice = pipe_msg_rcv.choice, client_id = pipe_msg_rcv.client_id;

            /*
            UNCOMMENT FOR DEBUGGING
            printf("client_id: %d   choice: %d\n", client_id, choice);
            */

            // Struct Variable to send the message to message queues
            struct msg_buf msg_snd;
            msg_snd.mtype = client_id + 2;
            msg_snd.msg.client_id = client_id;

            if (choice == 1) // Client choice is 1
            {

                // Send message with text "Hello" into message queue
                char hello[BUF_SIZE] = "Hello";
                msg_snd.msg.choice = choice;
                strncpy(msg_snd.msg.text, hello, BUF_SIZE);
                if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                {
                    perror("Error in sending message");
                    printf("%d\n", errno);
                    exit(EXIT_FAILURE);
                }
            }
            else if (choice == 2) // Client choice is 2
            {
                int pid2;
                char filename[BUF_SIZE];
                strncpy(filename, pipe_msg_rcv.ptext, BUF_SIZE);

                // Check for file existence
                if (!exists(filename)) // File does not exist
                {
                    char msg[BUF_SIZE] = "File not found";
                    strncpy(msg_snd.msg.text, msg, BUF_SIZE);

                    // Send the message to message queue
                    if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                    {
                        perror("Error in sending message");
                        printf("%d\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
                else // File exists
                {
                    char msg[BUF_SIZE] = "File exists";
                    strncpy(msg_snd.msg.text, msg, BUF_SIZE);

                    // Send the message to message queue
                    if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                    {
                        perror("Error in sending message");
                        printf("%d\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else if (choice == 3) // Client choice is 3
            {

                char filename[BUF_SIZE];
                // copy the received filename to local variable 'filename'
                strncpy(filename, pipe_msg_rcv.ptext, BUF_SIZE);

                // Check if file exists
                if (!exists(filename)) // Return error if file does not existss
                {
                    char msg[BUF_SIZE] = "File not found";

                    strncpy(msg_snd.msg.text, msg, BUF_SIZE);
                    if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                    {
                        perror("Error in sending message");
                        printf("%d\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {

                    // Declare a pipe to read output of execlp()
                    int pid3;
                    int fd3[2];
                    if (pipe(fd3) == -1)
                    {
                        perror("pipe creation failed");
                    }

                    // Create a child process to execute execlp
                    pid3 = fork();
                    char execlp_output[BUF_SIZE];

                    if (pid3 == 0)
                    {
                        close(fd3[0]);

                        // Redirect stdout to the pipe
                        dup2(fd3[1], STDOUT_FILENO);

                        // Execute wc -w command
                        if (execlp("wc", "wc", "-w", filename, NULL) == -1)
                        {
                            perror("Execlp failed");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        // Wait for child to finish execution
                        wait(NULL);

                        close(fd3[1]);

                        ssize_t word_count = read(fd3[0], execlp_output, BUF_SIZE);

                        // Check if the output was read correctly
                        if (word_count < 0)
                        {
                            perror("Failed to read from pipe");
                            exit(EXIT_FAILURE);
                        }

                        // Extract word count from execlp output
                        get_word_count(execlp_output);
                    }

                    // Send the word count message into message queue
                    strncpy(msg_snd.msg.text, execlp_output, BUF_SIZE);
                    if ((msgsnd(msg_q_id, &msg_snd, sizeof(msg_snd), 0)) == -1)
                    {
                        perror("Error in sending message");
                        printf("%d\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
            }

            exit(EXIT_SUCCESS);
        }
    }

    // Graceful termination
    cleanup(msg_q_id, fd[0], fd[1]);
    printf("Exit successful\n");

    return 0;
}