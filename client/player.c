#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>


#define GN 31
#define PLID "099298"
#define MAX_WORD_SIZE

/* Printing information to the user ------------------------------------------------------------------ */
#define GENERIC_ERROR "An error has occurred :(\nPlease try again!\n"
#define WELCOME_MSG "Welcome to the 'RC Word Game'! :D\nPlease start a new game by typing "\
                    "`start PLID`, where PLID is your student number.\n"
#define ONGOING_GAME "There is still an ongoing game. Please continue it (`play *letter*`) or `quit` instead\n"
#define STARTED_GAME "A new game has started! The word has >%d< letters and you have a maximum of >%d< errors!\n"
#define AVAILABLE_COMMANDS "\nThe following commands are available:\n"\
"-`play *letter*` or `pl *letter*` - plays a letter;\n"\
"-`guess *word*` or `gw *word*`    - guesses a word;\n"\
"-`scoreboard` or `sb`             - Displays the scoreboard;\n"\
"-`hint` or `h`                    - recieves a hint to help find the word;\n"\
"-`state` or `st`                  - recieves a summary of the state of the current game or the most recent game;\n"\
"-`quit`                           - quits the current game;\n"\
"-`exit`                           - quits the current game and exists the program.\n"
#define WELCOME_STATE "This is the current state of your game:\n"
#define SAVED_IMAGE "Imagem guardada com sucesso!\nA imagem encontra-se no ficheiro %s\n"
/* --------------------------------------------------------------------------------------------------- */

#define ERROR (-1)
#define START 1
#define PLAY 2
#define GUESS 3
#define SCOREBOARD 4
#define HINT 5
#define STATE 6
#define QUIT 7
#define EXIT 8
#define INPUT_SIZE 31
#define CHUNK_SIZE 1024
#define REV 9 // FIXME remover mais tarde

#define PROCESS_NOK (-3)
#define PROCESS_EMPTY (-4)

#define min(a, b) (((a) < (b)) ? (a) : (b))

int trials = 1;
char plid[] = "000000";
typedef struct game {
    int max_errors;
    int curr_errors;
    int nr_letters;
    char *current_word;
} game;

void print_current_word(game *game){
    printf("The current word is:\n%s\n", game->current_word);
}

/* Initializes some variables of the current game and informs the user */
int process_start(game *current_game, char *response){

    // Separates the response code
    char *splitted = strtok(response, " ");

    // The response code "RSG" is expected from the server
    if (strcmp(splitted, "RSG") != 0) {
        printf(GENERIC_ERROR);
        return -1;
    }

    // Separates the status code
    splitted = strtok(NULL, " ");

    // Processes the "NOK" status
    if (strcmp(splitted, "NOK") == 0) {
        printf(ONGOING_GAME);
        return -1;
    } // Checks for other status
    else if (strcmp(splitted, "OK") != 0) {
        printf(GENERIC_ERROR);
        return -1;
    }
    // Processes the "OK" Status, that is, a new game was created
    // Separates the number of letters and the maximum number of errors
    splitted = strtok(NULL, " ");
    current_game->nr_letters = atoi(splitted);

    splitted = strtok(NULL, " ");
    current_game->max_errors = atoi(splitted);
    printf(STARTED_GAME, current_game->nr_letters, current_game->max_errors);

    // Initializes the other variables
    current_game->curr_errors = 0;
    trials = 1;

    current_game->current_word = (char *) malloc(sizeof (char) * current_game->nr_letters);
    memset(current_game->current_word, '_', current_game->nr_letters);

    print_current_word(current_game);
    printf(AVAILABLE_COMMANDS);
    return 0;
}


/* sends a message and saves the message sent in buffer via udp protocol*/
int send_message_udp(char *ip, char* port, char* cmd, char* buffer) {

    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;

    struct addrinfo hints, *res;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    errcode = getaddrinfo(ip, port, &hints, &res);
    if (errcode != 0) {
        return -1;
    }

    n = sendto(fd, cmd, strlen(cmd), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        return -1;
    }

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, CHUNK_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
        return -1;
    }

    write(1, "DEBUG: ", 6);
    write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);

    return 0;
}


void printUsage() {
    printf("Usage: ./player [-n GSIP] [-p GSport]");
}


int read_word(char *buffer, int fd, int max_word_len){
    ssize_t nleft,nwritten, nread;
    char *ptr;
    int count;

    
    nleft=max_word_len; 
    ptr=buffer;
    while(nleft>0){
        nread =read(fd,ptr,1);
        if (*ptr == ' '){   
            *ptr = '\0';
            break;
        }
        else if(nread==-1)/*error*/exit(1);
        else if (nread == 0)
            break; // closed by peer
        nleft -= nread;
        ptr += nread;
    }
    nread = max_word_len - nleft;
    return nread;
}


// Reads a chunk of 1024 bytes from the server to the buffer
int read_chunk(char *buffer, int fd, int toRead){

    ssize_t nleft, n_read;
    n_read = read(fd,buffer,toRead);
    return n_read;
}


/* sends a message and saves the message sent in a file, via tcp protocol */
int send_message_tcp(char *ip, char* port, char* cmd) {

    int fd, errcode, filesize;
    ssize_t n;
    ssize_t nbytes,nleft,nwritten, nread;
    char *ptr;
    char *cmd_code = (char *) malloc(sizeof(char) * (3+1));
    char *status = (char *) malloc(sizeof(char) * (5+1));
    char *buffer = (char *) malloc(sizeof(char) * (CHUNK_SIZE));

    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;

    char filename[24 + 1];
    char filepath[24 + 1 + 5 + 1] = "files/";
    char filesize_str[10 + 1];


    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    errcode = getaddrinfo(ip, port, &hints, &res);
    if (errcode != 0) {
        exit(1);
    }

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }


    n=write(fd, cmd, strlen(cmd));
    if (n == -1) {
        exit(1);
    }

    /*
        n=read(fd, buffer, CHUNK_SIZE);
        if (n == -1) {
            exit(1);
        }

    */

    n = read_word(cmd_code, fd, 3+1);
    if (strcmp(cmd_code, "RSB") != 0 && strcmp(cmd_code, "RHL") != 0 && strcmp(cmd_code, "RST") != 0) {
        exit(1);
    }
    n = read_word(status, fd, 5+1);
    if (strcmp(status, "ACT") != 0 && strcmp(status, "FIN") != 0 && strcmp(status, "NOK") != 0 &&
        strcmp(status, "OK") != 0 && strcmp(status, "EMPTY") != 0) {
        exit(1);
    }

    if (strcmp(status, "EMPTY") == 0) {
        return PROCESS_EMPTY;
    }

    if (strcmp(status, "NOK") == 0) {
        return PROCESS_NOK;
    }

    // A file is read from the server, so we need to read the file name and then the file size, using the function read_word
    n = read_word(filename, fd, 24+1);
    if (n == -1) {
        exit(1);
    }

    n = read_word(filesize_str, fd, 10+1);
    if (n == -1) {
        exit(1);
    }

    filesize = atoi(filesize_str);

    // We know that the file has the name filename and the size filesize, so we can create a file with that name and size
    // If the cmd_code is RHL, we need to create a file with the name filename and the size filesize. Otherwise, we keep the information in a buffer. We should use fopen to open the file
    // and fwrite to write the file. We should use the function read_word to read the file from the server.
    if (strcmp(cmd_code, "RHL") == 0) {

        strcat(filepath, filename);
        int fd_img = open(filepath, O_RDWR | O_CREAT, 0666);
        if (fd_img == -1) {
            exit(1);
        }

        int toRead = filesize;

        // Reads the image from the server and saves it locally
        while (toRead > 0) {
            nread = read_chunk(buffer, fd, min(CHUNK_SIZE, toRead));
            write(fd_img, buffer, nread);
            toRead -= (int) nread;
        }

        printf(SAVED_IMAGE, filepath);
    }
    // Writes to the STDOUT
    else {
        int toRead = filesize;

        // Reads the image from the server and saves it locally
        while (toRead > 0) {
            nread = read_chunk(buffer, fd, min(CHUNK_SIZE, toRead));
            write(1, buffer, nread);
            toRead -= (int) nread;
        }
    }


    /* Imprime a mensagem "echo" e o conteúdo do buffer (ou seja, o que foi recebido
    do servidor) para o STDOUT (fd = 1) */
    write(1, "DEBUG: ", 6);
    write(1, status, strlen(status));
    write(1, "\n", 1);

    /* Desaloca a memória da estrutura `res` e fecha o socket */
    freeaddrinfo(res);
    close(fd);

    // Frees all the allocated buffers
    free(cmd_code);
    free(status);
    free(buffer);
}


/* Processes the input from the command line, attributing a new IP and a new Port if those arguments were passed */
int processInput(int argc, char *argv[], char *ip, char *port){
    
    if (argc > 5) 
        return -1;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            strcpy(ip, argv[i+1]);
        } else if (strcmp(argv[i], "-p") == 0) {
            strcpy(port, argv[i+1]);
        }
    }
    return 1;
}


/* Reads a command and it's argument (if applicable) from the user */
int readCommand(int *cmdCode, char *cmd){

    int res;
    char buffer[INPUT_SIZE]; // Creates a temporary buffer to read the command

    // Reads the command
    res = scanf("%s", buffer);
    if (res == -1)
        return -1;

    // Maps the command to an integer and constructs the full command

    if (strcmp(buffer, "start") == 0 || strcmp(buffer, "sg") == 0){

        *cmdCode = START;

        // Reads the PLID and saves it
        res = scanf("%s", buffer);
        if (res == -1)
            return -1;
        strcpy(plid, buffer);

        // Creates the SNG command in the format "SNG PLID"
        strcpy(cmd, "SNG ");
        strcat(cmd, buffer);
        strcat(cmd, "\n");
    }

    else if (strcmp(buffer, "play") == 0 || strcmp(buffer, "pl") == 0){

        *cmdCode = PLAY;

        // Reads the letter to be played
        res = scanf("%s", buffer);
        if (res == -1)
            return -1;

        // Creates the PLG command in the format "PLG PLID letter trial"
        strcpy(cmd, "PLG ");
        strcat(cmd, plid);
        strcat(cmd, " ");
        strcat(cmd, buffer);
        strcat(cmd, " ");
        sprintf(buffer, "%d", trials);
        strcat(cmd, buffer);
        strcat(cmd, "\n");
    }

    else if (strcmp(buffer, "guess") == 0 || strcmp(buffer, "gw") == 0){

        *cmdCode = GUESS;

        // Reads the word
        res = scanf("%s", buffer);
        if (res == -1)
            return -1;

        // Creates the PWG command in the format "PWG PLID word trial"
        strcpy(cmd, "PWG ");
        strcat(cmd, plid);
        strcat(cmd, " ");
        strcat(cmd, buffer);
        strcat(cmd, " ");
        sprintf(buffer, "%d", trials);
        strcat(cmd, buffer);
        strcat(cmd, "\n");
    }

    else if (strcmp(buffer, "scoreboard") == 0 || strcmp(buffer, "sb") == 0) {
        *cmdCode = SCOREBOARD;

        strcpy(cmd, "GSB\n");
    }

    else if (strcmp(buffer, "hint") == 0 || strcmp(buffer, "h") == 0) {

        *cmdCode = HINT;

        strcpy(cmd, "GHL ");
        strcat(cmd, plid);
        strcat(cmd, "\n");
    }

    else if (strcmp(buffer, "state") == 0 || strcmp(buffer, "st") == 0) {

        *cmdCode = STATE;

        strcpy(cmd, "STA ");
        strcat(cmd, plid);
        strcat(cmd, "\n");
    }

    else if (strcmp(buffer, "quit") == 0) {

        *cmdCode = QUIT;

        // Creates the QUT command in the format "QUT PLID"
        strcpy(cmd, "QUT ");
        strcat(cmd, plid);
        strcat(cmd, "\n");
    }

    else if (strcmp(buffer, "exit") == 0) {

        *cmdCode = EXIT;

        // Creates the QUT command in the format "QUT PLID"
        strcpy(cmd, "QUT ");
        strcat(cmd, plid);
        strcat(cmd, "\n");
    }

    // FIXME remover mais tarde - comando REV para revelar a palavra
    else if (strcmp(buffer, "rev") == 0) {

        *cmdCode = REV;
        strcpy(cmd, "REV ");
        strcat(cmd, plid);
        strcat(cmd, "\n");
    }

    else
        *cmdCode = ERROR;

    return 1;
}


int main(int argc, char *argv[]) {
    
    char* response = (char*) malloc(sizeof(char) * CHUNK_SIZE);
    int res, toExit = 0;
    int cmdCode;                                            // The Command that the user passes
    char *cmd = (char*) malloc(sizeof(char)*1024);      // The argument of the command, if it exists
    char* port = "58011";                                   // porta do tejo. Depois, substituir por 58000+GN;

    char *ip = malloc(sizeof("___.___.___.___"));
    strcpy(ip, "193.136.138.142"); // ip do tejo. Depois, substituir por 127.0.0.1.

    /* Initializes the structure Game */
    game *current_game = (game *) malloc(sizeof(game));
    trials = 1;
    current_game->current_word = malloc(sizeof(char) * MAX_WORD_SIZE + 1);

    // reads the input from the command line and defines a new IP and port if they were passed as arguments
    res = processInput(argc, argv, ip, port);
    if (res == -1) {
        printUsage();
        return -1;
    }

    // Welcomes the user
    printf(WELCOME_MSG);
    while (!toExit) {
        // reads the command and it's argument (if applicable), from the user and communicates with the server.
        readCommand(&cmdCode, cmd);

        // For each case, sends the message to the server and processes the response
        switch (cmdCode) {

            case START:
                res = send_message_udp(ip, port, cmd, response);
                if (res == -1) {
                    return -1;
                }
                process_start(current_game, response);
                break;

            case PLAY:
                //printf("Play! Arg: %s", cmd);
                send_message_udp(ip, port, cmd, response);
                trials++; // TODO se a resposta for INV, não incremento trials
                break;

            case GUESS:
                //printf("Guess! Arg: %s", cmd);
                send_message_udp(ip, port, cmd, response);
                trials++;

                break;

            case SCOREBOARD:
                printf("Scoreboard!\n");
                send_message_tcp(ip, port, cmd);
                break;

            case HINT:
                printf("Hint!\n");
                send_message_tcp(ip, port, cmd);
                break;

            case STATE:
                printf(WELCOME_STATE);
                send_message_tcp(ip, port, cmd);
                printf(AVAILABLE_COMMANDS);
                break;

            case QUIT:
                send_message_udp(ip, port, cmd, response);
                break;

            case EXIT:
                //printf("Exit!\n");
                send_message_udp(ip, port, cmd, response);
                toExit = 1;
                break;

            case REV:     // FIXME remover mais tarde - comando REV para revelar a palavra
                //printf("Rev!\n");
                send_message_udp(ip, port, cmd, response);
                break;

            default:
                printf("ERROR (ISTO É MEU, NÃO DO SERVER)!");
                break;
        }
    }

    return 0;
}



// SERVER (sockets)
// UDP -> sempre aberto
// TCP -> sempre aberto
// para cada cliente -> abre quando é estabelecida uma ligação, fecha terminada a ligação

// CLIENT (sockets)
// Os sockets abrem e fecham para cada comando (UDP e TCP)

/*
 * TODO
 * CLIENTE
 * - Funções chamadas dentro de um while, se der -1, podem decidir terminar ou dar uma mensagem de erro e de seguida ler o próximo comando
 * - Por timer em funções que acham que faz sentido
 *
 * SERVIDOR
 * - Por timer em funções que acham que faz sentido
 * - Funções chamadas dentro de um while. ser -1, passam para próximo pedido
 *
 * */

// TODO Ver mallocs e frees todos
