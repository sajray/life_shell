#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pwd.h>
#include <limits.h> // Use angle brackets, not "limits.h"

#define MAX_ARGS 64
#define SHELL_INPUT 1024

int main() {
    char input[SHELL_INPUT];         
    char *args[MAX_ARGS];          
    char hostname[HOST_NAME_MAX + 1];
    struct passwd *pw = getpwuid(getuid());
    char *username = pw ? pw->pw_name : "unknown";   
    char cwd[PATH_MAX];            

    while (1) {
        // --- Advanced Prompt ---
        if (gethostname(hostname, sizeof(hostname)) != 0) {
            strcpy(hostname, "unknown");
        }
        getcwd(cwd, sizeof(cwd));
        char *home = getenv("HOME");

        if (home && strstr(cwd, home) == cwd) {
            printf("%s@%s:~%s$ ", username, hostname, cwd + strlen(home));
        } else {
            printf("%s@%s:%s$ ", username, hostname, cwd);
        }
        fflush(stdout);

        // --- Read Input ---
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0'; // strip newline
        if (strlen(input) == 0) continue;   // skip empty lines

        // --- Parse input into arguments ---
        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] == NULL) continue;

        // --- Handle "exit" command ---
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        // --- Handle "cd" command ---
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                // No argument -> go to home
                if (chdir(getenv("HOME")) != 0) {
                    perror("cd");
                }
            } else {
                // cd [path]
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
            continue; // Don't fork for cd
        }

        // --- Fork & Execute ---
        pid_t pid = fork();

        if (pid == 0) {
            execvp(args[0], args); // Child process
            perror("Command failed");
            exit(1);
        } else if (pid > 0) {
            wait(NULL); // Parent waits
        } else {
            perror("Fork failed");
        }
    }

    return 0;
}
