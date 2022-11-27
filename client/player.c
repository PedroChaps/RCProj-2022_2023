#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GN 31
#define PLID "099298"

#define ERROR (-1)
#define START 1
#define PLAY 2
#define GUESS 3
#define SCOREBOARD 4
#define HINT 5
#define STATE 6
#define QUIT 7
#define EXIT 8

void printUsage() {
    printf("Usage: ./player [-n GSIP] [-p GSport]");
}

void auxPrintArray(char **array) {
    int size = sizeof(array);
    int i;
    
    printf("[");
    for (i = 0; i < size; i++) {
        printf("%s, ", array[i]);
    }
    printf("]");
}

/* Processes the input from the command line, attributing a new IP and a new Port if those arguments were passed */
int processInput(int argc, char *argv[], char *ip, int *port){
    
    if (argc > 5) 
        return -1;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            strcpy(ip, argv[i+1]);
        } else if (strcmp(argv[i], "-p") == 0) {
            *port = atoi(argv[i+1]);
        }
    }
    return 1;
}

/* Reads a command and it's argument (if applicable) from the user */
int readCommand(int *cmd, char *cmdArg){

    int res, toReadArg = 0;
    char cmdString[11]; // Creates a temporary buffer to read the command

    // Reads the command
    res = scanf("%s", cmdString);
    if (res == -1)
        return -1;

    // Maps the command to an integer and reads an argument (if applicable)
    if (strcmp(cmdString, "start") == 0 || strcmp(cmdString, "sg") == 0){
        *cmd = START;
        toReadArg = 1;
    }

    else if (strcmp(cmdString, "play") == 0 || strcmp(cmdString, "pl") == 0){
        *cmd = PLAY;
        toReadArg = 1;
    }

    else if (strcmp(cmdString, "guess") == 0 || strcmp(cmdString, "gw") == 0){
        *cmd = GUESS;
        toReadArg = 1;
    }

    else if (strcmp(cmdString, "scoreboard") == 0 || strcmp(cmdString, "sb") == 0)
        *cmd = SCOREBOARD;

    else if (strcmp(cmdString, "hint") == 0 || strcmp(cmdString, "h") == 0)
        *cmd = HINT;

    else if (strcmp(cmdString, "state") == 0 || strcmp(cmdString, "st") == 0)
        *cmd = STATE;

    else if (strcmp(cmdString, "quit") == 0)
        *cmd = QUIT;

    else if (strcmp(cmdString, "exit") == 0)
        *cmd = EXIT;

    else
        *cmd = ERROR;

    // If it's necessary to read an argument, reads it
    if (toReadArg){
        res = scanf("%s", cmdArg);
        if (res == -1)
            return -1;
    }

    return 1;
}
        
int main(int argc, char *argv[]) {
    
    int res;
    int cmd;                         // The Command that the user passes
    char *cmdArg = malloc(31);  // The argument of the command, if it exists
    int port = 58011;                // porta do tejo. Depois, substituir por 58000+GN;
    char *ip = malloc(sizeof("___.___.___.___"));
    strcpy(ip, "193.136.138.142"); // ip do tejo. Depois, substituir por 127.0.0.1.

    // reads the input from the command line and defines a new IP and port if they were passed as arguments
    res = processInput(argc, argv, ip, &port);
    if (res == -1) {
        printUsage();
        return -1;
    }

    while (1) {
        // reads the command and it's argument (if applicable), from the user and communicates with the server
        readCommand(&cmd, cmdArg);
        switch (cmd) {

            case START:
                printf("Start! Arg: %s\n", cmdArg);
                break;

            case PLAY:
                printf("Play! Arg: %s\n", cmdArg);
                break;

            case GUESS:
                printf("Guess! Arg: %s\n", cmdArg);
                break;

            case SCOREBOARD:
                printf("Scoreboard!\n");
                break;

            case HINT:
                printf("Hint!\n");
                break;

            case STATE:
                printf("State!\n");
                break;

            case QUIT:
                printf("Quit!\n");
                break;

            case EXIT:
                printf("Exit!\n");
                break;

            default:
                printf("ERROR!\n");
                break;
        }
    }
}