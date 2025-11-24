// pipe.c
// WAP to create two child processes which execute commands passed as
// command-line arguments, separated by '|'.
// First child: executes command 1, sends output to pipe.
// Second child: reads from pipe, executes command 2.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[])
{
    // 1. No arguments passed
    if (argc == 1) {
        fprintf(stderr, "Error: No arguments passed\n");
        fprintf(stderr, "Usage: ./pipe <command 1> | <command 2>\n");
        return 1;
    }

    // Find index of '|' in argv
    int pipe_index = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }

    // 2. '|' not found or insufficient args on either side
    if (pipe_index == -1 || pipe_index == 1 || pipe_index == argc - 1) {
        fprintf(stderr, "Error: Insufficient arguments passed\n");
        fprintf(stderr, "Usage: ./pipe <command 1> | <command 2>\n");
        return 1;
    }

    // Split argv into two command arrays:
    // cmd1 = argv[1..pipe_index-1]
    // cmd2 = argv[pipe_index+1..argc-1]

    argv[pipe_index] = NULL;  // terminate first command arguments
    char **cmd1 = &argv[1];
    char **cmd2 = &argv[pipe_index + 1];

    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid1, pid2;

    // First child: executes command 1, writes to pipe
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        return 1;
    }

    if (pid1 == 0) {
        // Child 1
        // Redirect stdout to pipe write end
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        close(fd[0]); // close unused read end
        close(fd[1]); // close original write end

        execvp(cmd1[0], cmd1);
        // If execvp returns, it’s an error
        perror("execvp (command 1)");
        exit(1);
    }

    // Second child: executes command 2, reads from pipe
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        return 1;
    }

    if (pid2 == 0) {
        // Child 2
        // Redirect stdin to pipe read end
        if (dup2(fd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        close(fd[1]); // close unused write end
        close(fd[0]); // close original read end

        execvp(cmd2[0], cmd2);
        // If execvp returns, it’s an error
        perror("execvp (command 2)");
        exit(1);
    }

    // Parent: close both ends of the pipe and wait for children
    close(fd[0]);
    close(fd[1]);

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    return 0;
}
