#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
//#include "../lib/unp.h"
#include "unp.h" 

// find the length the of the string, ignore all NULL characters
int stringSize(char *str) {
    int length = 0;
    char* ptr = str;
    while (*ptr != '\0' && *ptr != '\n' && *ptr != '\r' && *ptr) {
        length++;
        ptr++;
    }
    return length;
}
//removes the carriage return from the string
void remove_newline(char *str) {
    char* ptr = str;
    while (*ptr != '\r' && *ptr) {
        ptr++;
    }

    if (*ptr == '\r' || *ptr == '\n') {
        *ptr = '\0';
    }
}
//returns a string that is the lowercase of the input string
char* tolower_string(char* str) {
    char* cpy = str;
    for(int i = 0; *(str+i); i++){
        *(str + i) = tolower(*(str + i));
    }
    return cpy;
}

int strings_position_match(char* word, char* word2) {
    int num_match = 0;
    for (int i = 0; i < stringSize(word) && *(word2 + i); i++) {
        if (tolower(*(word + i)) == tolower(*(word2 + i))) num_match++;
    }
    return num_match;
}
//returns the number of letters that 
int strings_letters_match(char* word, char* word2) {
    int letters[26] = {0};
    int letters2[26] = {0};

    for (int i = 0; i < stringSize(word); i++) {
        letters[tolower(word[i]) - 'a']++;
    }

    for (int i = 0; i < stringSize(word2); i++) {
        letters2[tolower(word2[i]) - 'a']++;
    }

    int num_match = 0;
    for (int i = 0; i < 26; i++) {
        num_match += min(letters[i], letters2[i]);
    }

    return num_match;
}

// close a socket and removes the username and password used
void close_socket(int* server_socks, char** usernames, int i) {
    close(server_socks[i]);
    server_socks[i] = 0;
    free(usernames[i]);
    usernames[i] = NULL;
}
//save the word file to an array
void saveWords(char** dict, FILE* inputFile, char* word, int MAX_LEN){
    // reset file pointer if necessary
    fseek(inputFile, 0, SEEK_SET);

    int dict_idx = 0;
    while (fgets(word, MAX_LEN, inputFile)) {
        tolower_string(word);
        *(dict+dict_idx) = calloc(MAX_LEN, sizeof(char));
        *(word + stringSize(word)) = '\0';
        strcpy(*(dict+dict_idx), word);
        dict_idx++;
    }
}

int initServer(struct sockaddr_in* server, int port){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Error setting SO_REUSEADDR");
        return 1;
    }
    bzero(&server, sizeof(server));
    server -> sin_family      = AF_INET;
    server -> sin_addr.s_addr = INADDR_ANY;
    server -> sin_port        = htons(port);

    if (bind(sockfd, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("select error");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 3) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int main(int argc, char ** argv ) {
    if (argc != 5) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: ./word_guess.out [seed] [port] [dictionary_file] [longest_word_length]\n");
        return EXIT_FAILURE;
    }

    int seed = atoi(*(argv+1));
    int port = atoi(*(argv+2));
    char* filepath = *(argv+3);
    int MAX_LEN = atoi(*(argv+4)) + 1;

    char msg[MAXLINE];

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: INVALID FILE PATH\n");
        return EXIT_FAILURE;
    }

    char* word = calloc(MAX_LEN, sizeof(char));
    int num_words = 0;
    while(fgets(word, MAX_LEN, file)) {
        num_words++;
    }
    // save all valid words
    char** dict = calloc(num_words, sizeof(char*));
    saveWords(dict, file, word, MAX_LEN);
    // set random seed
    srand( seed );

    char* hidden_word = dict[rand() % num_words];
    // saved sockets and usernames each client
    int* server_socks = calloc(5, sizeof(int));
    char** usernames = calloc(5, sizeof(char*));

    int					sockfd;
    struct sockaddr_in	servaddr, cliaddr;
    sockfd = initServer(&servaddr, port);

    socklen_t cli_addr_size;

    fd_set readfds, reads;
    // Initialize the file descriptor set
    FD_ZERO(&reads);
    FD_SET(sockfd, &reads); // Add stdin to set

    int num_connected = 0;
    char* buffer = calloc(MAXLINE, sizeof(char));

    bool guess_correct = false;
    //start the game
    while (true) {
        // one of the users found the secret word; select another word
        if (guess_correct) {
            hidden_word = dict[rand() % num_words];
            // reset read file descriptors
            FD_ZERO(&reads);
            FD_SET(sockfd, &reads);
            guess_correct = false; 
        }
        readfds = reads;

        // Check if any of the connected servers have sent data
        int num_ready = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if (num_ready < 0) {
            perror("select error");
            exit(1);
        }

        if (FD_ISSET(sockfd, &readfds)) { 
            if (num_connected < 5) {
                int newsockfd = accept(sockfd, (struct sockaddr *) &cliaddr, &cli_addr_size);
                sprintf(msg, "Welcome to Guess the Word, please enter your username.\n");
                send(newsockfd, msg, strlen(msg), 0);
                // find the empty socket pos
                for (int i = 0; i < 5; i++) {
                    if (server_socks[i] == 0) {
                        server_socks[i] = newsockfd;
                        break;
                    }
                }

                FD_SET(newsockfd, &reads);
                num_connected++;
            }
            else {
                int newsockfd = accept(sockfd, (struct sockaddr *) &cliaddr, &cli_addr_size);
                close(newsockfd);
            }
        }
        for (int i = 0; i < 5; i++) {
            if (server_socks[i] > 0 && FD_ISSET(server_socks[i], &readfds)) {
                //  at least one server sent a message
                int n = recv(server_socks[i], buffer, MAXLINE - 1, 0);
                if (n == 0) {
                    FD_CLR(server_socks[i], &reads);
                    close_socket(server_socks, usernames, i);
                    num_connected--;
                } 
                else if (n > 0) {
                    remove_newline(buffer);
                    buffer[stringSize(buffer)] = '\0';
                        
                    if (usernames[i] == NULL) {
                        // user is setting the username
                        char* username = calloc(strlen(buffer), sizeof(char));
                        strcpy(username, buffer);

                        // check if the username is already in use
                        bool duplicate = false;
                        for (int j = 0; j < 5; j++) {
                            if (usernames[j] == NULL) continue;
                            
                            // compare the usernames without modifying the original input
                            char* username_in_use = calloc(strlen(usernames[j]), sizeof(char));
                            strcpy(username_in_use, usernames[j]);
                            if (strcmp(tolower_string(username_in_use), tolower_string(buffer)) == 0) {
                                duplicate = true;
                            }
                            free(username_in_use);
                        }
                       
                        if (!duplicate) {
                            usernames[i] = username;
                            sprintf(msg, "Let's start playing, %s\n", usernames[i]);
                            send(server_socks[i], msg, strlen(msg), 0);    
                            sprintf(msg, "There are %d player(s) playing. The secret word is %d letter(s).\n", num_connected, stringSize(hidden_word));
                        }
                        else {
                            sprintf(msg, "Username %s is already taken, please enter a different username\n", username);
                        }
                        send(server_socks[i], msg, strlen(msg), 0);                      
                    }
                    else {
                        // user is guessing the word
                        int guess_len = stringSize(buffer);
                        if (guess_len != stringSize(hidden_word)) {
                            sprintf(msg, "Invalid guess length. The secret word is %d letter(s).\n", stringSize(hidden_word));
                            send(server_socks[i], msg, strlen(msg), 0);
                        }
                        else {
                            // send guess result to every other client
                            int exact_match = strings_position_match(hidden_word, buffer);
                            int letters_match = strings_letters_match(hidden_word, buffer);

                            // check if the guess is correct
                            if (letters_match == stringSize(hidden_word)) {
                                sprintf(msg, "%s has correctly guessed the word %s", usernames[i], hidden_word);
                                guess_correct = true;
                            }
                            else {
                                sprintf(msg, "%s guessed %s: %d letter(s) were correct and %d letter(s) were correctly placed.\n", usernames[i], buffer, letters_match, exact_match);
                            }

                            for (int j = 0; j < 5; j++) {
                                if (usernames[j] == NULL) continue;
                                send(server_socks[j], msg, strlen(msg), 0);

                                if (guess_correct) {
                                    // close the socket after a user guessed the secret word
                                    FD_CLR(server_socks[i], &reads);
                                    close_socket(server_socks, usernames, j);
                                    num_connected--;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    free(server_socks);
    free(buffer);
    for (int i = 0; i < 5; i++) {
        free(*(usernames + i));
    }
    free(usernames);
    free(word);
    for (int i = 0; i < num_words; i++) {
        free(*(dict + i));
    }
    free(dict);
    fclose(file);

    return EXIT_SUCCESS;
}