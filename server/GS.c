#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#define CHUNK_SIZE 1024


extern int errno;

int v = 0;
char word_file[25];
int word_file_size = 0;
char hint_file[25];


void printUsage() {
    printf("Usage: ./GS word_file [-p GSport] [-v]");
}


int processInput(int argc, char *argv[], char *port) {

    if (argc > 5)
        return -1;

    // checks if the word file exists in the filesystem
    if (access(argv[1], F_OK) == -1) {
        printf("Error: word file does not exist\n");
        return -1;
    }
    strcpy(word_file, argv[1]);

    // Saves the number of lines of the file
    FILE *fp = fopen(word_file, "r");
    char c;
    while ((c = (char) fgetc(fp)) != EOF) {
        if (c == '\n') {
            word_file_size++;
        }
    }

    // Checks for the -p and -v flags
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            strcpy(port, argv[i+1]);
        } else if (strcmp(argv[i], "-v") == 0) {
            v = 1;
        }
    }
    return 1;
}

/* Given a word, returns the number of letters and the maximum errors */
int get_nr_letters_and_errors(char *word, int *nr_letters, int *max_errors) {

    *nr_letters = (int) strlen(word);

    if (*nr_letters <= 6) {
        *max_errors = 7;
    } else if (*nr_letters >= 7 && *nr_letters <= 10) {
        *max_errors = 8;
    } else if (*nr_letters >= 11) {
        *max_errors = 9;
    }

    return 0;
}

/* processes the command SNG from the client */
int start_game(char *command, char *response){

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int *nr_letters = (int *) malloc(sizeof(int));
    int *max_errors = (int *) malloc(sizeof(int));

    // extracts the PLID from the command
    command = strtok(NULL, " ");
    command[strlen(command) - 1] = '\0';
    char *plid = command;

    // builds the file path
    char filepath[6 + 15 + 1] = "games/game_";
    strcat(filepath, plid);
    strcat(filepath, ".txt");

    // checks if the player has any ongoing game with atleast one move.
    // if he does, then he cannot start a new game.
    // First, checks if the file exits. If it does, checks if it has any play.
    if (access(filepath, F_OK) != -1) {

        fp = fopen(filepath, "r");
        if (fp == NULL) {
            return -1;
        }

        // Reads two lines. If the second line is not empty, then the player has a game in progress.
        read = getline(&line, &len, fp);
        read = getline(&line, &len, fp);
        if (read > 0) {
            strcpy(response, "RSG NOK\n");
            fclose(fp);
            return 1;
        }

        fclose(fp);
    }

    // If the file already existed but the player didn't make a play,
    // the player is just informed of the number of letters and the maximum errors allowed.
    if (access(filepath, F_OK) != -1) {

        fp = fopen(filepath, "r");
        if (fp == NULL) {
            return -1;
        }

        char *word = (char *) malloc(sizeof(char) * (read + 1));
        if (word == NULL) {
            return -1;
        }

        // Reads the first line and gets the word, extracting it from the line
        read = getline(&line, &len, fp);
        word = strtok(line, " ");

        get_nr_letters_and_errors(word, nr_letters, max_errors);

        // Builds the response, sending the number of letters and the maximum errors allowed
        strcpy(response, "RSG OK ");
        char nr_letters_str[3];
        sprintf(nr_letters_str, "%d", *nr_letters);
        strcat(response, nr_letters_str);
        strcat(response, " ");
        char max_errors_str[3];
        sprintf(max_errors_str, "%d", *max_errors);
        strcat(response, max_errors_str);

        fclose(fp);
        return 0;
    }

    // If the file doesn't exist, then it is created and the player is informed of the number
    // of letters and the maximum errors allowed.

    // Gets a random word and hint from the word file
    int random_line = rand() % word_file_size;
    fp = fopen(word_file, "r");
    if (fp == NULL) {
        return -1;
    }
    for (int i = 0; i < random_line; i++) { // skips the lines until the random line
        read = getline(&line, &len, fp);
    }

    // splits the random line and saves the name and hint in the file
    char *splitted = (char *) malloc(sizeof(char) * (read + 1));
    char *word = (char *) malloc(sizeof(char) * (30 + 1));
    char *hint = (char *) malloc(sizeof(char) * (24 + 1));
    if (splitted == NULL || word == NULL || hint == NULL) {
        return -1;
    }

    splitted = strtok(line, " ");
    strcpy(word, splitted);

    splitted = strtok(NULL, " ");
    strcpy(hint, splitted);

    fclose(fp);

    // Saves the information in the new file
    fp = fopen(filepath, "w");
    if (fp == NULL) {
        return -1;
    }

    fputs(word, fp);
    fputs(" ", fp);
    fputs(hint, fp);

    fclose(fp);

    // Builds the response (`RSG OK n_letters max_errors`),
    // sending the number of letters and the maximum errors allowed
    get_nr_letters_and_errors(word, nr_letters, max_errors);

    strcpy(response, "RSG OK ");
    char nr_letters_str[2 + 1]; // max 2 digits + \0
    sprintf(nr_letters_str, "%d", *nr_letters);
    strcat(response, nr_letters_str);
    strcat(response, " ");
    char max_errors_str[1 + 1]; // max 1 digit + \0
    sprintf(max_errors_str, "%d", *max_errors);
    strcat(response, max_errors_str);

    return 0;
}


/* Recieves a command from the client and process it. Saves what is to be sent back to the client in `response` */
int process_client_message(char *command, char *response){

    // Reads the command code
    char *splitted = strtok(command, " ");

    // Choses based on the command code
    if (strcmp(splitted, "SNG") == 0) {
        return start_game(command, response);
    }

    // TODO implementar estas meninas
    //    else if (strcmp(splitted, "PLG") == 0) {
    //        return play_letter(command, response);
    //    }
    //
    //    else if (strcmp(splitted, "PWG") == 0) {
    //        return guess_word(command, response);
    //    }
    //
    //    else if (strcmp(splitted, "GSB") == 0) {
    //        return get_scoreboard(command, response);
    //    }
    //
    //    else if (strcmp(splitted, "GHL") == 0) {
    //        return get_hint(command, response);
    //    }
    //
    //    else if (strcmp(splitted, "STA") == 0) {
    //        return get_state(command, response);
    //    }
    //
    //    else if (strcmp(splitted, "QUT") == 0) {
    //        return quit_game(command, response);
    //    }
    //
    //    else if (strcmp(splitted, "REV") == 0) {
    //        return reveal_word(command, response);
    //    }
    //
    //    else {
    //        TODO implementar um erro
    //        strcpy(response, "ERR");
    //        return -1;
    //    }

    return 0;

}

int process_messages_UDP(char *port){

    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128] = {0};
    char *response = (char *) malloc(CHUNK_SIZE * sizeof(char));

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0) {
        exit(1);
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        printf("%d", errno);
        printf("%s\n", strerror(errno));
        exit(1);
    }

    while (1) {
        // Recieves a message from the client
        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
        if (n == -1) {
            exit(1);
        }

        // FIXME Debug message, remove later
        write(1, "DEBUG: ", 6);
        write(1, buffer, n);
        write(1, "\n", 1);

        // Processes the message recieved
        process_client_message(buffer, response);


        // Sends the response to the client. The maximum size is CHUNK_SIZE
        n = sendto(fd, response, strlen(response), 0, (struct sockaddr *)&addr, addrlen);
        if (n == -1) {
            exit(1);
        }

        // Resets the memory of `response`
        memset(response, 0, CHUNK_SIZE);
    }

    freeaddrinfo(res);
    close(fd);
}


int process_messages_TCP(char *port){

    int fd, newfd, errcode, pid, ret; // newfd é fd da nova ligação (existem 2 sockets em TCP)
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128], *ptr;
    ssize_t nw;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if ((errcode) != 0) {
        exit(1);
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }

    if (listen(fd, 5) == -1) { // TODO perguntar quantas conexões podem estar pendentes
        exit(1);
    }

    /* Loop para processar uma socket de cada vez */
    while (1) {

        // wait for a new connection
        do {
            newfd = accept(fd, (struct sockaddr *) &addr, &addrlen);
        } while (newfd == -1 && errno == EINTR);
        if (newfd == -1) {
            printf("%d", errno);
            printf("%s\n", strerror(errno));
            exit(1);
        }

        // fork a new process to handle the new connection
        if ((pid = fork()) == -1)
            exit(1);

        // child process
        else if (pid == 0) {
            close(fd);
            while ((n = read(newfd, buffer, 128)) != 0) {
                if (n == -1) /*error*/
                    exit(1);
                ptr = &buffer[0];
                while (n > 0) {
                    if ((nw = write(newfd, ptr, n)) <= 0) /*error*/
                        exit(1);
                    n -= nw;
                    ptr += nw;
                }
            }
            close(newfd);
            exit(0);
        }

        // parent process - closes the new socket and waits for another connection
        do {
            ret = close(newfd);
        } while (ret == -1 && errno == EINTR);
        if (ret == -1)
            exit(1);
    }

    freeaddrinfo(res);
    close(fd);

    return 0;
}



int main(int argc, char *argv[]) {

    int res;
    char *port = (char *) malloc(sizeof(char) * 6);
    time_t t;
    srand((unsigned) time(&t));
    strcpy(port, "58011");

    res = processInput(argc, argv, port);
    if (res == -1) {
        printUsage();
        return -1;
    }

    // Child process
    if (fork() == 0) {
        process_messages_TCP(port);
    } else {
        process_messages_UDP(port);
    }

    return 0;
}

/* TODO
 * [ ] - Implementar o -v (verbose)
 *
 * */