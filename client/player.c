#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

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


/* sends a message and saves the message sent in buffer*/
int send_message_udp(char *ip, char* port, char* cmd, char* arg_cmd, char* buffer, int flag) {
    

    char* hostname;
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen; // Tamanho do endereço

    /*
    hints - Estrutura que contém informações sobre o tipo de conexão que será estabelecida.
            Podem-se considerar, literalmente, dicas para o sistema operacional sobre como
            deve ser feita a conexão, de forma a facilitar a aquisição ou preencher dados.

    res - Localização onde a função getaddrinfo() armazenará informações sobre o endereço.
    */
    struct addrinfo hints, *res;
    struct sockaddr_in addr;

    /* Cria um socket UDP (SOCK_DGRAM) para IPv4 (AF_INET).
    É devolvido um descritor de ficheiro (fd) para onde se deve comunicar. */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        exit(1);
    }

    /* Preenche a estrutura com 0s e depois atribui a informação já conhecida da ligação */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    /* Busca informação do host "localhost", na porta especificada,
    guardando a informação nas `hints` e na `res`. Caso o host seja um nome
    e não um endereço IP (como é o caso), efetua um DNS Lookup. */
    errcode = getaddrinfo(ip, port, &hints, &res);
    if (errcode != 0) {
        exit(1);
    }


    /* Envia para o `fd` (socket) a mensagem "Hello!\n" com o tamanho 7.
       Não são passadas flags (0), e é passado o endereço de destino.
       É apenas aqui criada a ligação ao servidor. */
    n = sendto(fd, "SNG 099298\n", 11, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }

    /* Recebe 128 Bytes do servidor e guarda-os no buffer.
       As variáveis `addr` e `addrlen` não são usadas pois não foram inicializadas. */
    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
        exit(1);
    }

    /* Imprime a mensagem "echo" e o conteúdo do buffer (ou seja, o que foi recebido
    do servidor) para o STDOUT (fd = 1) */
    write(1, "echo: ", 6);
    write(1, buffer, n);

    /* Desaloca a memória da estrutura `res` e fecha o socket */
    freeaddrinfo(res);
    close(fd);
}



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
    
    char* response = (char*) malloc(sizeof(char)*1024);
    int res;
    int cmd;                         // The Command that the user passes
    char *cmdArg = malloc(31);      // The argument of the command, if it exists
    char* port = "58011";           // porta do tejo. Depois, substituir por 58000+GN;

    char *ip = malloc(sizeof("___.___.___.___"));
    strcpy(ip, "193.136.138.142"); // ip do tejo. Depois, substituir por 127.0.0.1.

    // reads the input from the command line and defines a new IP and port if they were passed as arguments
    res = processInput(argc, argv, ip, port);
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
                send_message_udp(ip, port, "SNG", cmdArg, response, 1);
                printf("%s\n", response);
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



// SERVER (sockets)
// UDP -> sempre aberto
// TCP -> sempre aberto
// para cada cliente -> abre quando é estabelecida uma ligação, fecha terminada a ligação

// CLIENT (sockets)
// Os sockets abrem e fecham para cada comando (UDP e TCP)