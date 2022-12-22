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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/signal.h>
#define CHUNK_SIZE 1024
#define min(a, b) (((a) < (b)) ? (a) : (b))


extern int errno;

int v = 0;
char word_file[25];
int word_file_size = 0;
char hint_file[25];


void printUsage() {
    printf("Usage: ./GS word_file [-p GSport] [-v]");
}

// Returns the number of lines of a file (including the first one!)
int get_nr_lines(char *filename) {
    FILE *fp = fopen(filename, "r");
    char c;
    int lines = 0;
    while ((c = (char) fgetc(fp)) != EOF) {
        if (c == '\n') {
            lines++;
        }
    }
    fclose(fp);
    return lines;

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

    word_file_size = get_nr_lines(word_file);

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
    char filepath[6 + 15 + 1] = "GAMES/game_";
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
    for (int i = 0; i <= random_line; i++) { // skips the lines until the random line
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
    hint[strlen(hint) - 1] = '\0';

    fclose(fp);

    // Saves the information in the new file
    fp = fopen(filepath, "w");
    if (fp == NULL) {
        return -1;
    }

    fputs(word, fp);
    fputs(" ", fp);
    fputs(hint, fp);
    fputs(" 0\n", fp); // saves the number of errors

    fclose(fp);

    // Create the curr_word_PLID.txt file
    char currword_filepath[27] = "GAMES/curr_word_";
    strcat(currword_filepath, plid);
    strcat(currword_filepath, ".txt");
    fp = fopen(currword_filepath, "w");
    if (fp == NULL) {
        return -1;
    }

    // Sets `-` for each letter of the word
    for (int i = 0; i < strlen(word); i++) {
        fputs("-", fp);
    }
    fputs("\0", fp);
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

int get_timestamp(char *buffer){

    time_t rawtime;
    struct tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);

    strftime(buffer,80,"%Y%m%d_%H%M%S",timeinfo);

    return 0;
}

int move_and_rename(char *source_path, char *dst_dir, char *code) {

    // Check if the destination directory exists
    struct stat st;
    if (stat(dst_dir, &st) == -1) {
        // If the directory does not exist, create it
        mkdir(dst_dir, 0700);
    }

    // calculates the filename of the file to be moved
    char *filename = (char *) malloc(sizeof(char) * (strlen("YYYYMMDD_HHMMSS_X.txt") + 1));
    if (filename == NULL) {
        return -1;
    }
    get_timestamp(filename); // gets the timestamp
    strcat(filename, "_"); // adds the underscore
    strcat(filename, code); // adds the code
    strcat(filename, ".txt"); // adds the extension

    // Create the full path to the destination file by concatenating the directory and filename
    strcat(dst_dir, filename);

    // use the rename function to move the file
    int result = rename(source_path, dst_dir);
    if (result != 0) {
        perror("Error renaming file");
    }

    // Free the dynamically allocated memory
    // free(filename); TODO: ver o que se passa aqui porque está a dar "munmap_chunk(): invalid pointer"

    return 0;
}

int quit_game(char *command, char *response) {

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // extracts the PLID from the command
    command = strtok(NULL, " ");
    command[strlen(command) - 1] = '\0';
    char *plid = command;

    // builds the file path
    char filepath[26 + 1] = "GAMES/game_";
    strcat(filepath, plid);
    strcat(filepath, ".txt");

    // checks if the player has any ongoing game with atleast one move.
    // if he does, then remove the file and send a response.
    if (access(filepath, F_OK) == 0){

        // Creates the variables to call `move_and_rename()`
        char *folderpath = (char *) malloc(sizeof(char) * (6 + 15 + 1));
        if (folderpath == NULL) {
            return -1;
        }
        strcpy(folderpath, "GAMES/");
        strcat(folderpath, plid);
        strcat(folderpath, "/");

        // Move the file to the `GAMES/PLID` folder
        move_and_rename(filepath, folderpath, "Q");
        //free(folderpath); TODO: Queixa-se que o free está a ser feito em memória não alocada

        // Removes the `curr_word_PLID.txt` file
        strcpy(filepath, "GAMES/curr_word_");
        strcat(filepath, plid);
        strcat(filepath, ".txt");
        remove(filepath);

        strcpy(response, "RQT OK\n");
        return 0;
    }
    else if (access(filepath, F_OK) == -1) {
        strcpy(response, "RQT NOK\n");
        return 1;
    }
    else{
        strcpy(response, "RQT ERR\n");
        return 1;
    }
}



int play_letter(char *command, char *response) {
    FILE *fp;
    char *line = NULL;
    char letter; // letter to be played
    char letter_read; // letter read from the file
    char word_read[30+1]; // word read from the file
    char plid[6+1];
    int positions[30]; // positions of the letter in the word
    size_t len = 0;
    int trial;
    int trial_server;
    char buffer_aux[2+1]; //max size of a trial + \0
    char code;
    char *curr_word = (char *) malloc(sizeof(char) * (30 + 1));
    int i, nr_errors;
    int *nr_letters_word = (int *) malloc(sizeof(int));
    int *max_errors = (int *) malloc(sizeof(int));
    ssize_t read;

    // initializes the array with -1
    memset(positions, -1, sizeof(positions));

    // extracts the PLID, letter and trial from the command
    command = strtok(NULL, " ");
    strcpy(plid, command);

    command = strtok(NULL, " ");
    letter = command[0];

    command = strtok(NULL, " ");
    trial = atoi(command);

    // builds the file path and the curr_game path
    char filepath[26 + 1] = "GAMES/game_";
    strcat(filepath, plid);
    strcat(filepath, ".txt");

    char curr_game_path[26 + 1] = "GAMES/curr_word_";
    strcat(curr_game_path, plid);
    strcat(curr_game_path, ".txt");

    // opens the file. If it fails, it's because the player doesn't have an ongoing game, so returns an error
    fp = fopen(filepath, "r");
    if (fp == NULL) {
        strcpy(response, "RLG ERR\n");  // TODO check if PLID and PWG syntax are valid
        return -1;
    }

    // reads the first line of the file, which contains the word to be guessed as well as the number of errors
    // copies them to the `word_read` and the `nr_erros` variable
    read = getline(&line, &len, fp);
    command = strtok(line, " ");
    strcpy(word_read, command);
    command = strtok(NULL, " ");
    command = strtok(NULL, "\0");
    command[strlen(command) - 1] = '\0';
    nr_errors = atoi(command);

    // gets the number of trials of the server, by reading the number of lines in the file
    trial_server = get_nr_lines(filepath); // default + 1

    // checks if the trial from the client is valid
    if (trial != trial_server){
        strcpy(response, "RLG INV ");
        sprintf(buffer_aux, "%d", trial_server);
        strcat(response, buffer_aux);
        strcat(response, "\n");
        return -1;
    }

    // checks for a duplicate letter
    while (getline(&line, &len, fp) > 0){
        // Checks for the code `T` (trial). If it's a trial, compares the letter to the one in the file
        code = *line;
        if (code == 'T'){

            // Extracts the letter from the line
            command = strtok(line, " ");
            command = strtok(NULL, " ");
            letter_read = command[0];

            // If they are equal, then it's a duplicate letter
            if (letter_read == letter){
                strcpy(response, "RLG DUP ");
                sprintf(buffer_aux, "%d", trial_server);
                strcat(response, buffer_aux);
                strcat(response, "\n");
                return -1;
            }
        }
    }
    fclose(fp);

    // Opens the file to append (register) the last play
    fp = fopen(filepath, "a");
    if (fp == NULL) {
        return -1;
    }
    fprintf(fp, "T %c\n", letter);
    if (fclose(fp) != 0) {
        return -1;
    }

    // Open the curr_game_`plid`.txt file to read the current state of the word, and copies it to a buffer
    fp = fopen(curr_game_path, "r");
    if (fp == NULL) {
        return -1;
    }
    read = getline(&line, &len, fp);
    if (read == -1) {
        return -1;
    }
    strcpy(curr_word, line);
    fclose(fp);

    int nr_letters = 0;
    // Checks if the letter exists in the word
    for (i = 0; i < strlen(word_read); i++){
        if (word_read[i] == letter){
            curr_word[i] = letter;
            positions[nr_letters++] = i;
       }
    }

    // If the letter doesn't exist in the word, then the response is `NOK`
    if (nr_letters == 0){

        // Checks for the number of errors
        get_nr_letters_and_errors(word_read, nr_letters_word, max_errors);
        if (nr_errors >= *max_errors) {
            // Creates the variables to call `move_and_rename()`
            char *folderpath = (char *) malloc(sizeof(char) * (6 + 15 + 1));
            if (folderpath == NULL) {
                return -1;
            }
            strcpy(folderpath, "GAMES/");
            strcat(folderpath, plid);
            strcat(folderpath, "/");


            // Move the file to the `GAMES/PLID` folder
            move_and_rename(filepath, folderpath, "F");

            // Removes the `curr_word_PLID.txt` file
            strcpy(filepath, "GAMES/curr_word_");
            strcat(filepath, plid);
            strcat(filepath, ".txt");
            remove(filepath);

            // Creates the response
            strcpy(response, "RLG OVR ");
            sprintf(buffer_aux, "%d", trial_server);
            strcat(response, buffer_aux);
            strcat(response, "\n");
            return 0;
        }

        // There were still errors remaining, so increments the errors
        // Opens the file to increment the number of errors
        fp = fopen(filepath, "r+");  // Open the file for reading and writing
        if (fp == NULL) {
            return -1;
        }

        // Move the file pointer to the beginning of the file
        if (fseek(fp, 0, SEEK_SET) != 0)
            return -1;

        // Read the first line of the file
        char buffer[1024];
        fgets(buffer, 1024, fp);
        i = (int) strlen(buffer) - 1;
        // Modify the contents of the buffer to contain the new first line of the file
        while(buffer[i] != ' '){
            i--;
        }
        i++; // i is now the position of the first digit of the number of errors
        sprintf(buffer_aux, "%d", nr_errors + 1);
        strcpy(buffer + i, buffer_aux);

        // Seek back to the beginning of the file and write the modified buffer to the file
        fseek(fp, 0, SEEK_SET);
        fputs(buffer, fp);

        fclose(fp);  // Close the file

        // Creates the `NOK` response
        strcpy(response, "RLG NOK ");
        sprintf(buffer_aux, "%d", trial_server);
        strcat(response, buffer_aux);
        strcat(response, "\n");
        return 0;
    }

    // Otherwise, the response is `OK`
    // Save the buffer to the file
    fp = fopen(curr_game_path, "w");
    if (fp == NULL) {
        return -1;
    }
    fprintf(fp, "%s", curr_word);
    fclose(fp);

    // Checks if there are no `-` in the word, which means that the word was guessed
    if (strchr(curr_word, '-') == NULL){
        // Creates the variables to call `move_and_rename()`
        char *folderpath = (char *) malloc(sizeof(char) * (6 + 15 + 1));
        if (folderpath == NULL) {
            return -1;
        }
        strcpy(folderpath, "GAMES/");
        strcat(folderpath, plid);
        strcat(folderpath, "/");

        // Move the file to the `GAMES/PLID` folder
        move_and_rename(filepath, folderpath, "W");

        // Removes the `curr_word_PLID.txt` file
        strcpy(filepath, "GAMES/curr_word_");
        strcat(filepath, plid);
        strcat(filepath, ".txt");
        remove(filepath);

        // Creates the response
        strcpy(response, "RLG WIN ");
        sprintf(buffer_aux, "%d", trial_server);
        strcat(response, buffer_aux);
        strcat(response, "\n");
        return 0;
    }

    // Create the dynamic response
    strcpy(response, "RLG OK ");
    sprintf(buffer_aux, "%d", trial_server);
    strcat(response, buffer_aux);
    strcat(response, " ");
    sprintf(buffer_aux, "%d", nr_letters);
    strcat(response, buffer_aux);
    strcat(response, " ");
    for (i = 0; i < nr_letters && positions[i] != -1; i++){
        sprintf(buffer_aux, "%d", positions[i] + 1);
        strcat(response, buffer_aux);
        strcat(response, " ");
    }
    // Removes the last space
    response[strlen(response) - 1] = '\0';
    strcat(response, "\n");

    return 0;
}

/* Finds and writes the top scores to the file path `writepath` */
int write_top_scores(char *writepath) {

    struct dirent **filelist;
    int n_entries, i_file;
    char fname[50];
    FILE *fp_sb, *fp;

    n_entries = scandir("SCORES/", &filelist, 0, alphasort);
    // open the file to write the top scores
    fp_sb = fopen(writepath, "w");
    if (fp_sb == NULL) {
        return -1;
    }
    fputs("-------------------------------- TOP 10 SCORES --------------------------------\n\n", fp_sb);
    fputs("    SCORE PLAYER     WORD                             GOOD TRIALS  TOTAL TRIALS\n\n", fp_sb);

    i_file = 0;
    if (n_entries < 0) {
        return 0;
    }
    else {
        while (n_entries--) {
            if (filelist[n_entries]->d_name[0] != '.') {
                sprintf(fname, "SCORES/%s", filelist[n_entries]->d_name);
                fp = fopen(fname, "r");
                if (fp != NULL) {

                    // Reads the only line of the file
                    char buffer[1024];
                    fgets(buffer, 1024, fp);

                    fprintf(fp_sb, "%-2d - ", i_file + 1);

                    // Splits the line into tokens
                    char *token = strtok(buffer, " ");
                    fprintf(fp_sb, "%s  ", token);

                    token = strtok(NULL, " ");
                    fprintf(fp_sb, "%s  ", token);

                    token = strtok(NULL, " ");
                    fprintf(fp_sb, "%-35s ", token);

                    token = strtok(NULL, " ");
                    fprintf(fp_sb, "     %-10s ", token);

                    token = strtok(NULL, " ");
                    fprintf(fp_sb, "  %-10s\n", token);

                    ++i_file;
                    fclose(fp);
                }
            }
            free(filelist[n_entries]);
            if (i_file == 10)
                break;
        }
        free(filelist);
    }
    fclose(fp_sb);
    return i_file;
}


int guess_word(char *command, char *response) {
    FILE *fp;
    char *line = NULL;
    char word[30+1];     // word read from the command
    char word_read[30+1]; // word read from the file
    char plid[6+1];
    size_t len = 0;
    ssize_t read;
    int trial;
    int trial_server;
    char buffer_aux[2+1]; //max size of a trial + \0
    char code;
    char *word_guessed = (char *) malloc(sizeof(char) * (30 + 1));
    int *nr_letters = (int *) malloc(sizeof(int));
    int *max_errors = (int *) malloc(sizeof(int));
    int nr_errors, i;


    // extracts the PLID, word and trial from the command
    command = strtok(NULL, " ");
    strcpy(plid, command);

    command = strtok(NULL, " ");
    strcpy(word, command);

    command = strtok(NULL, " ");
    trial = atoi(command);

    // builds the file path
    char filepath[26 + 1] = "GAMES/game_";
    strcat(filepath, plid);
    strcat(filepath, ".txt");

    // opens the file. If it fails, it's because the player doesn't have an ongoing game, so returns an error
    fp = fopen(filepath, "r");
    if (fp == NULL) {
        strcpy(response, "RWG ERR\n");  // TODO check if PLID and PWG syntax are valid
        return -1;
    }

    read = getline(&line, &len, fp);
    command = strtok(line, " ");
    strcpy(word_read, command);
    command = strtok(NULL, " ");
    command = strtok(NULL, "\0");
    command[strlen(command) - 1] = '\0';
    nr_errors = atoi(command);

    trial_server = get_nr_lines(filepath); // default + 1


    if (trial != trial_server){
        strcpy(response, "RWG INV ");
        sprintf(buffer_aux, "%d", trial_server);
        strcat(response, buffer_aux);
        strcat(response, "\n");
        return -1;
    }

    while (getline(&line, &len, fp) > 0){
        code = *line;
        if (code == 'G'){
            command = strtok(line, " ");
            command = strtok(NULL, " ");
            strcpy(word_guessed, command);
            word_guessed[strlen(word_guessed) - 1] = '\0';
            printf("word_guessed: %s\n", word_guessed);

            if (strcmp(word, word_guessed) == 0){
                strcpy(response, "RWG DUP ");
                sprintf(buffer_aux, "%d", trial_server);
                strcat(response, buffer_aux);
                strcat(response, "\n");
                return -1;
            }
        }
    }
    fclose(fp);

    // Opens the file to append (register) the last play
    fp = fopen(filepath, "a");
    if (fp == NULL) {
        return -1;
    }
    fprintf(fp, "G %s\n", word);

    if (fclose(fp) != 0) {
        return -1;
    }

    // Checks if the word is correct
    if (strcmp(word, word_read) == 0){

        // Creates the variables to call `move_and_rename()`
        char *folderpath = (char *) malloc(sizeof(char) * (6 + 15 + 1));
        if (folderpath == NULL) {
            return -1;
        }
        strcpy(folderpath, "GAMES/");
        strcat(folderpath, plid);
        strcat(folderpath, "/");

        // Move the file to the `GAMES/PLID` folder
        move_and_rename(filepath, folderpath, "W");

        // Removes the `curr_word_PLID.txt` file
        strcpy(filepath, "GAMES/curr_word_");
        strcat(filepath, plid);
        strcat(filepath, ".txt");
        remove(filepath);


        // Calculates the score
        char score_buffer[3+1];
        int n_succ = trial_server - nr_errors;
        int score = ((100 * n_succ) / trial_server);
        if (score > 100 || score < 0){
            printf("YOU GOT BAD SCORE MAN\n");
            return -1;
        }

        // convert score to string
        sprintf(score_buffer, "%d", score);
        // prepend 0s to score until it's exacly 3 digits
        //char *aux = (char *) malloc(sizeof(char) * (3 + 1));
        char aux[3+1];
        while (strlen(score_buffer) < 3){
            strcpy(aux, score_buffer);
            strcpy(score_buffer, "0");
            strcat(score_buffer, aux);
            memset(aux, 0, strlen(aux));
        }


        // Create a file "SCORES/SCORE_PLID_DDMMYYYY_HHMMSS.txt"
        char *score_filepath = (char *) malloc(sizeof(char) * (37 + 1));
        if (score_filepath == NULL) {
            return -1;
        }
        strcpy(score_filepath, "SCORES/");
        strcat(score_filepath, score_buffer);
        strcat(score_filepath, "_");
        strcat(score_filepath, plid);
        strcat(score_filepath, "_");
        get_timestamp(score_filepath + strlen(score_filepath));
        strcat(score_filepath, ".txt");

        // Opens the file to write "score PLID word n_succ n_trials"
        fp = fopen(score_filepath, "w+");
        if (fp == NULL) {
            return -1;
        }


        //fputs(score_filepath, fp);
        fprintf(fp, "%s %s %s %d %d",score_buffer, plid, word_read, n_succ, trial_server);

        // Closes the file
        if (fclose(fp) != 0) {
            return -1;
        }

        // Creates the response
        strcpy(response, "RWG WIN ");
        sprintf(buffer_aux, "%d", trial_server);
        strcat(response, buffer_aux);
        strcat(response, "\n");
        return 0;
    }

    // Checks for the number of errors
    get_nr_letters_and_errors(word_read, nr_letters, max_errors);
    if (nr_errors >= *max_errors) {
        // Creates the variables to call `move_and_rename()`
        char *folderpath = (char *) malloc(sizeof(char) * (6 + 15 + 1));
        if (folderpath == NULL) {
            return -1;
        }
        strcpy(folderpath, "GAMES/");
        strcat(folderpath, plid);
        strcat(folderpath, "/");


        // Move the file to the `GAMES/PLID` folder
        move_and_rename(filepath, folderpath, "F");

        // Removes the `curr_word_PLID.txt` file
        strcpy(filepath, "GAMES/curr_word_");
        strcat(filepath, plid);
        strcat(filepath, ".txt");
        remove(filepath);

        // Creates the response
        strcpy(response, "RWG OVR ");
        sprintf(buffer_aux, "%d", trial_server);
        strcat(response, buffer_aux);
        strcat(response, "\n");
        return 0;
    }

    // Opens the file to increment the number of errors
    fp = fopen(filepath, "r+");  // Open the file for reading and writing
    if (fp == NULL) {
        return -1;
    }

    // Move the file pointer to the beginning of the file
    if (fseek(fp, 0, SEEK_SET) != 0)
        return -1;

    // Read the first line of the file
    char buffer[1024];
    fgets(buffer, 1024, fp);
    i = (int) strlen(buffer) - 1;
    // Modify the contents of the buffer to contain the new first line of the file
    while(buffer[i] != ' '){
        i--;
    }
    i++; // i is now the position of the first digit of the number of errors
    sprintf(buffer_aux, "%d", nr_errors + 1);
    strcpy(buffer + i, buffer_aux);

    // Seek back to the beginning of the file and write the modified buffer to the file
    fseek(fp, 0, SEEK_SET);
    fputs(buffer, fp);

    fclose(fp);  // Close the file

    // Creates and sends the response
    strcpy(response, "RWG NOK ");
    sprintf(buffer_aux, "%d", trial_server);
    strcat(response, buffer_aux);
    strcat(response, "\n");
    return 1;

}

// Reads a chunk of 1024 bytes from the server to the buffer
int read_chunk(char *buffer, int fd, int toRead){

    ssize_t nleft, n_read;
    n_read = read(fd,buffer,toRead);
    return n_read;
}


// Finds the last game of a given plid and saves the the filepath in fname
int FindLastGame(char *PLID, char *fname)
{
    struct dirent **filelist;
    int n_entries, found;
    char dirname[20];
    sprintf(dirname, "GAMES/%s/", PLID);

    n_entries = scandir(dirname, &filelist, 0, alphasort);
    found = 0;
    if (n_entries <= 0)
        return (0);
    else
    {
        while (n_entries--)
        {
            if (filelist[n_entries]->d_name[0] != '.')
            {
                sprintf(fname, "GAMES/%s/%s", PLID, filelist[n_entries]-> d_name);
                found = 1;
            }
            free(filelist[n_entries]);
            if (found)
                break;
        }
        free(filelist);
    }
    return (found);
}

// processes the state command when there are no ongoing games
int process_state_quit(char* plid, int fd){
    char *filepath = (char*) malloc( sizeof(char) * (strlen("GAMES/099181/20221219_04:22:16_W.txt")+1) );
    FILE *fp_game = NULL;
    char *buffer = (char *) malloc(sizeof(char) * CHUNK_SIZE);
    char *word = (char*) malloc(sizeof(char) * (30+1));
    char *hint_name = (char*) malloc(sizeof(char) * (24+1));
    char *line = NULL;
    size_t len = 0;
    int nr_lines;
    char *response = (char *) malloc(sizeof(char) * CHUNK_SIZE);
    int nread;

    char *state_path = (char *) malloc(sizeof(char) * (22 + 1));
    if (state_path == NULL) {
        return -1;
    }

    strcpy(state_path, "GAMES/state_");
    strcat(state_path, plid);
    strcat(state_path, ".txt");


    FILE *fp_state = fopen(state_path, "w");
    if (fp_state== NULL) {
        return -1;
    }
    fprintf(fp_state, "\tLast finalized game for player %s\n", plid);
    strcpy(buffer, "\t--- Transactions found: ");
    sprintf(buffer + strlen(buffer), "%d", nr_lines);
    strcat(buffer, " ---\n");
    fputs(buffer, fp_state);

    // finds the last game of the player and saves the plid gamepath in filepath
    if(FindLastGame(plid, filepath) == 0){
        return -1;
    }

    nr_lines = get_nr_lines(filepath) - 1; // -1 because of the header
    
    // opens the file
    fp_game = fopen(filepath, "r");
    if(fp_game == NULL){
        return -1;
    }

    // reads the first line of the file
    fgets(buffer, 1024, fp_game);

    // reads the word to guess
    char *splitted = strtok(buffer, " ");
    strcpy(word, splitted);


    // reads the hint_name
    splitted = strtok(NULL, " ");
    strcpy(hint_name, splitted);

    // writes the word and the hint_name to the state file
    fprintf(fp_state, "\tWord: %s; Hint file: %s\n", word, hint_name);

    // set the buffer to all 0s
    memset(buffer, 0, CHUNK_SIZE);


    while(getline(&line, &len, fp_game) != -1){
        strcpy(buffer, "\t");
        // If the first letter is T, writes "Letter trial: `letter`"
        if(line[0] == 'T') {
            strcat(buffer, "Letter trial: ");
            char trial = line[2];
            sprintf(buffer + strlen(buffer), "%c", trial);
            strcat(buffer, " - ");
            // If the letter is in the word, appends "TRUE". Otherwise, appends "FALSE"
            if(strchr(word, line[2]) != NULL) {
                strcat(buffer, "TRUE");
            }
            else {
                strcat(buffer, "FALSE");
            }
            strcat(buffer, "\n");
        }
        else if (line[0] == 'G'){
            strcat(buffer, "Word guess: ");
            strcat(buffer, line + 2);
        }
        fputs(buffer, fp_state);
    }
        fclose(fp_game);

    memset(buffer, 0, sizeof(char) * CHUNK_SIZE);
/* 
    // Use the stat function to get information about the file
    struct stat file_stat;
    if (stat(state_path, &file_stat) != 0) {
        perror("Error getting file status");
        return 1;
    }
    int state_size = file_stat.st_size; */

    // Gets the file size
    fseek(fp_state, 0L, SEEK_END);
    int state_size = ftell(fp_state);
    fclose(fp_state);

    // Sends the output to the client
    strcpy(response, "RST ACT ");
    strcat(response, "state_");
    strcat(response, plid);
    strcat(response, ".txt ");
    sprintf(buffer, "%d ", state_size);
    strcat(response, buffer);

    write(fd, response, strlen(response));

    // appends to the response buffer the contents of the hint file in chuncks of 1024 bytes using write_chunk()
    int toRead = state_size;

    // opens the state file and associates it to a File Descriptor
    int fd_state = open(state_path, O_RDONLY);
    if (fd_state == -1) {
        return -1;
    }

    // Reads the file in chunks of 1024 bytes and writes it to the client
    while (toRead > 0) {
        nread = read_chunk(buffer, fd_state, min(CHUNK_SIZE, toRead));
        write(fd, buffer, nread);
        toRead -= (int) nread;
    }
    // closes the file descriptor
    close(fd_state);

    return 0;

}


// TODO: FAZER PARA QUANDO NÃO HÁ JOGO ATIVO -> FAZER ACCESS() PARA VER SE O FICHEIRO EXISTE; DIVIDIR EM FUNÇÕES
int process_state(char *command, int fd){

    char *game_path = (char *) malloc(sizeof(char) * (19 + 1));
    memset(game_path, 0, sizeof(char) * (19 + 1));
    char *curr_word_path = (char *) malloc(sizeof(char) * (24 + 1));
    memset(curr_word_path, 0, sizeof(char) * (24 + 1));
    char curr_word[30+1];
    memset(curr_word, 0, sizeof(char) * (30 + 1));
    char *buffer = (char *) malloc(sizeof(char) * CHUNK_SIZE);
    memset(buffer, 0, sizeof(char) * CHUNK_SIZE);
    char *response = (char *) malloc(sizeof(char) * CHUNK_SIZE);
    memset(response, 0, sizeof(char) * CHUNK_SIZE);
    int nr_lines = -1;
    int nread;
    char *line = NULL;
    size_t len = 0;

    // extracts the PLID
    command = strtok(NULL, " ");
    command[6] = '\0';

    // saves PLID in a variable
    char *plid = (char *) malloc(sizeof(char) * (6 + 1));
    if (plid == NULL) {
        return -1;
    }
    strcpy(plid, command);

    // Opens a file to write the output of state, which will be then sent to the server
    char *output_path = (char *) malloc(sizeof(char) * (22 + 1));
    if (output_path == NULL) {
        return -1;
    }

    // open the file `GAMES/game_PLID.txt` to extract the plays
    strcpy(game_path, "GAMES/game_");
    strcat(game_path, plid);
    strcat(game_path, ".txt");

    // if the file doens't exist, go to function process_state_quit
    if (access(game_path, F_OK) == -1) {
        process_state_quit(plid, fd);
        return 0;
    }

    FILE *fp_game = fopen(game_path, "r");
    if (fp_game == NULL) {
        return -1;
    }

    strcpy(output_path, "GAMES/state_");
    strcat(output_path, plid);
    strcat(output_path, ".txt");
    FILE *fp_output = fopen(output_path, "w");
    if (fp_output == NULL) {
        return -1;
    }

    // open `GAMES/curr_word_PLID.txt` to extract the current state of the word
    strcpy(curr_word_path, "GAMES/curr_word_");
    strcat(curr_word_path, plid);
    strcat(curr_word_path, ".txt");
    FILE *fp_curr_word = fopen(curr_word_path, "r");
    if (fp_curr_word == NULL) {
        return -1;
    }
    fgets(curr_word, 30, fp_curr_word);
    fclose(fp_curr_word);

    // count how many lines there are on the `GAMES/game_PLID.txt` file. They are the number of trials
    nr_lines = get_nr_lines(game_path);

    // ignores the first line of `GAMES/game_PLID.txt`
    getline(&line, &len, fp_game);

    // Creates the output and sends it line by line
    strcpy(buffer, "This is the current state of your game:\n");
    fputs(buffer, fp_output);

    strcpy(buffer, "\tActive game found for player ");
    strcat(buffer, plid);
    strcat(buffer, "\n");
    fputs(buffer, fp_output);

    strcpy(buffer, "\t--- Transactions found: ");
    sprintf(buffer + strlen(buffer), "%d", nr_lines);
    strcat(buffer, " ---\n");
    fputs(buffer, fp_output);

    while(getline(&line, &len, fp_game) != -1){
        strcpy(buffer, "\t");
        // If the first letter is T, writes "Letter trial: `letter`"
        if(line[0] == 'T') {
            strcat(buffer, "Letter trial: ");
            char trial = line[2];
            sprintf(buffer + strlen(buffer), "%c", trial);
            strcat(buffer, " - ");
            // If the letter is in the word, appends "TRUE". Otherwise, appends "FALSE"
            if(strchr(curr_word, line[2]) != NULL) {
                strcat(buffer, "TRUE");
            }
            else {
                strcat(buffer, "FALSE");
            }
            strcat(buffer, "\n");
        }
        else if (line[0] == 'G'){
            strcat(buffer, "Word guess: ");
            strcat(buffer, line + 2);
        }
        fputs(buffer, fp_output);
    }
    // Puts the current word in the output
    strcpy(buffer, "\tSolved so far: ");
    strcat(buffer, curr_word);
    strcat(buffer, "\n");
    fputs(buffer, fp_output);
    memset(buffer, 0, sizeof(char) * CHUNK_SIZE);

    // Gets the file size
    fseek(fp_output, 0L, SEEK_END);
    int state_size = ftell(fp_output);
    fclose(fp_output);

    // Sends the output to the client
    strcpy(response, "RST ACT ");
    strcat(response, "state_");
    strcat(response, plid);
    strcat(response, ".txt ");
    sprintf(buffer, "%d ", state_size);
    strcat(response, buffer);

    write(fd, response, strlen(response));

    // appends to the response buffer the contents of the hint file in chuncks of 1024 bytes using write_chunk()
    int toRead = state_size;

    // opens the state file and associates it to a File Descriptor
    int fd_state = open(output_path, O_RDONLY);
    if (fd_state == -1) {
        return -1;
    }

    // Reads the image from the server and saves it locally
    while (toRead > 0) {
        nread = read_chunk(buffer, fd_state, min(CHUNK_SIZE, toRead));
        write(fd, buffer, nread);
        toRead -= (int) nread;
    }
    // closes the file descriptor
    close(fd_state);

    return 0;
    // free plid, fp_game
}




int process_hint(char *command, int fd){

    // extracts the PLID
    command = strtok(NULL, " ");
    command[6] = '\0';
    char *hint_name = (char *) malloc(sizeof(char) * (24 + 1));
    char *hint_path = (char *) malloc(sizeof(char) * (30 + 1));
    int nread;
    char *splitted = (char *) malloc(sizeof(char) * 1024);
    char *plid = (char *) malloc(sizeof(char) * (6 + 1));
    if (plid == NULL) {
        return -1;
    }
    strcpy(plid, command);


//    // extracts the PLID from the command
//    command = strtok(NULL, " ");
//    command[strlen(command) - 1] = '\0';
//    char *plid = command;

    // create the filepath GAMES/game_PLID.txt
    char *filepath = (char *) malloc(sizeof(char) * (20+1));
    if (filepath == NULL) { 
        return -1;
    }
    strcpy(filepath, "GAMES/game_");
    strcat(filepath, plid);
    strcat(filepath, ".txt");

    // open the file and read the second word from the first line and saves it in hint_name
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) { //TODO: N ESQUEÇER MUDAR
        // writes the error message to the client
        char *response = (char *) malloc(sizeof(char) * (strlen("RHL NOK\n") + 1));
        if (response == NULL) {
            return -1;
        }
        strcpy(response, "RHL NOK\n");
        write(fd, response, strlen(response));
        // TODO: remover, só para testes
        //  write(fd, response, strlen(response));
        free(response);
        return -1;
    }
    char buffer[1024];
    fgets(buffer, 1024, fp);
    fclose(fp);
    splitted = strtok(buffer, " ");
    splitted = strtok(NULL, " ");
    strcpy(hint_name, splitted);
    memset(buffer, 0, 1024);
    // create the full path (`HINTS/` + hint_name)
    strcpy(hint_path, "HINTS/");
    strcat(hint_path, hint_name);

    // get the size of the file
    fp = fopen(hint_path, "r");
    if (fp == NULL){
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    int hint_size = ftell(fp);
    fclose(fp);

    //writes the response to the client, containing "RHL OK ",the name of the hint file, the size of the hint file
    char *response = (char *) malloc(sizeof(char) * (strlen("RHL OK ") + strlen(hint_name) + 10 + 1));
    if (response == NULL) {
        return -1;
    }

    strcpy(response, "RHL OK ");
    strcat(response, hint_name);
    strcat(response, " ");
    sprintf(buffer, "%d ", hint_size);
    strcat(response, buffer);

    // sends the response to the client
    write(fd, response, strlen(response));

    // appends to the response buffer the contents of the hint file in chuncks of 1024 bytes using write_chunk()
    int toRead = hint_size;

    // opens the hint file and associates it to a File Descriptor
    int fd_hint = open(hint_path, O_RDONLY);
    if (fd_hint == -1) {
        return -1;
    }

    // Reads the image from the server and saves it locally
    while (toRead > 0) {
        nread = read_chunk(buffer, fd_hint, min(CHUNK_SIZE, toRead));
        write(fd, buffer, nread);
        toRead -= (int) nread;
    }
    // closes the file descriptor
    close(fd_hint);

}



int process_scoreboard(char *command, int fd){

    int nread, nr_entries;

    // Gets the filepath to write the scoreboard to
    char *sb_path = (char *) malloc(sizeof(char) * (14 + 1));
    if (sb_path == NULL) {
        return -1;
    }
    strcpy(sb_path, "scoreboard.txt");

    // Allocates the memory to the response buffer
    char *response = (char *) malloc(sizeof(char) * (strlen("RSB OK ") + strlen(sb_path) + 10 + 1));
    if (response == NULL) {
        return -1;
    }

    // Calls the function to compute the new scoreboard. If there are no entries, sends a response with "RSB EMPTY".
    nr_entries = write_top_scores(sb_path);
    if (nr_entries == 0) {
        strcpy(response, "RHL EMPTY\n");
        // sends the response to the client
        write(fd, response, strlen(response));
    }

    // get the size of the file
    FILE* fp = fopen(sb_path, "r");
    if (fp == NULL){
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    int sb_size = ftell(fp);
    fclose(fp);

    // appends to the response buffer the contents of the hint file in chuncks of 1024 bytes using write_chunk()
    int toRead = sb_size;

    //writes the response to the client, containing "RSB OK ",the name of the hint file, the size of the hint file
    char buffer[1024];
    strcpy(response, "RSB OK ");
    strcat(response, sb_path);
    strcat(response, " ");
    sprintf(buffer, "%d ", sb_size);
    strcat(response, buffer);

    // sends the response to the client
    write(fd, response, strlen(response));

    // opens the scoreboard file and associates it to a File Descriptor
    int fd_hint = open(sb_path, O_RDONLY);
    if (fd_hint == -1) {
        return -1;
    }

    // Reads the scoreboard and sends it to the client in chunks of 1024 bytes
    while (toRead > 0) {
        nread = read_chunk(buffer, fd_hint, min(CHUNK_SIZE, toRead));
        write(fd, buffer, nread);
        toRead -= (int) nread;
    }

    // Removes the file `scoreboard.txt`
    remove("scoreboard.txt");

    free(sb_path);
}





/* Recieves a command from the client and process it. Saves what is to be sent back to the client in `response` */
int process_client_message(char *command, char *response, int fd){

    // Reads the command code
    char *splitted = strtok(command, " ");

    // Choses based on the command code

    // UDP FUNCTIONS
    if (strcmp(splitted, "SNG") == 0) {
        return start_game(command, response);
    }
    else if (strcmp(splitted, "PLG") == 0) {
        return play_letter(command, response);
    }
    else if (strcmp(splitted, "PWG") == 0) {
        return guess_word(command, response);
    }
    else if (strcmp(splitted, "QUT") == 0) {
        return quit_game(command, response);
    }

    // TCP FUNCTIONS
    else if (strcmp(splitted, "GSB\n") == 0) {
        return process_scoreboard(command, fd);
     }
    else if (strcmp(splitted, "GHL") == 0) {
        return process_hint(command, fd);     //TODO: criar abstração a enviar ficheiro
    }
    else if (strcmp(splitted, "STA") == 0) {
     return process_state(command, fd);
    }


//    else {
//        TODO: implementar um erro
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
        exit(2);
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        printf("%d", errno);
        printf("%s\n", strerror(errno));
        exit(3);
    }

    while (1) {
        // Recieves a message from the client
        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
        if (n == -1) {
            exit(4);
        }

        // FIXME Debug message, remove later
        write(1, "DEBUG_UDP: ", 9);
        write(1, buffer, n);

        // Processes the message recieved
        process_client_message(buffer, response, -1);


        // Sends the response to the client. The maximum size is CHUNK_SIZE
        n = sendto(fd, response, strlen(response), 0, (struct sockaddr *)&addr, addrlen);
        if (n == -1) {
            exit(5);
        }

        // Resets the memory of `response` and 'command'
        memset(response, 0, CHUNK_SIZE);
        memset(buffer, 0, 128);
    }

    freeaddrinfo(res);
    close(fd);
}


// Writes a chunk of 1024 bytes from the server to the buffer
int write_chunk(char *buffer, int fd, int toRead){

    ssize_t nleft, n_read;
    n_read = write(fd,buffer,toRead);
    return n_read;
}

int process_messages_TCP(char *port){

    int fd, newfd, errcode, pid, ret; // newfd é fd da nova ligação (existem 2 sockets em TCP)
    ssize_t n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128], *ptr;
    ssize_t nw;
    char *chunk = (char *) malloc(CHUNK_SIZE * sizeof(char));

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        exit(6);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if ((errcode) != 0) {
        exit(7);
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        printf("%d\n", errno);
        printf(" %s\n", strerror(errno));
        exit(8);
    }

    if (listen(fd, 5) == -1) { // TODO perguntar quantas conexões podem estar pendentes
        exit(9);
    }

    /* Loop para processar uma socket de cada vez */
    while (1) {

        socklen_t addrlen = sizeof(addr);
        // wait for a new connection
        newfd = accept(fd, (struct sockaddr *) &addr, &addrlen);
        if (newfd == -1 && errno == EINTR) {
            printf("%d", errno);
            printf(" %s\n", strerror(errno));
            exit(10);
        }

        // fork a new process to handle the new connection
        if ((pid = fork()) == -1)
            exit(11);

        // parent process
        else if (pid > 0) {
            close(fd);
            while ((n = read(newfd, buffer, 128)) != 0) {
                // FIXME Debug message, remove later
                write(1, "DEBUG_TCP: ", 9);
                write(1, buffer, n);

                if (n == -1) /*error*/
                    exit(12);
                // Processes the message recieved
                process_client_message(buffer, chunk, newfd);

            }
            close(newfd);
            exit(0);
        }

        // child process
        else if (pid == 0) {
            // closes the new socket and waits for another connection
            do {
                ret = close(newfd);
            } while (ret == -1 && errno == EINTR);
            if (ret == -1)
                exit(14);
            }
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
    strcpy(port, "58031");

    res = processInput(argc, argv, port);
    if (res == -1) {
        printUsage();
        return -1;
    }
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    // signal(SIGINT, SIG_IGN); // TRATAR O SIGINT (nao com ign)

    // Creates the initial directories, if they are not created yet
    struct stat st;
    if (stat("GAMES", &st) == -1) {
        mkdir("GAMES", 0700);
    }
    if (stat("SCORES", &st) == -1) {
        mkdir("SCORES", 0700);
    }
    if (stat("HINTS", &st) == -1) {
        mkdir("HINTS", 0700);
    }

    // Child process
    if (fork() == 0) {
        process_messages_UDP(port);
    } else {
        process_messages_TCP(port);
    }

    return 0;
}

/* TODO:
 * [ ] - Implementar o -v (verbose)
 * [ ] - FAZER MAKEFILE
 * [ ] - Relatório maybe?
 * [ ] - Autoavaliação
 * */