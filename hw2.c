#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include "../../lib/unp.h"
// #include "unp.h" 

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

void remove_newline(char *str) {
    char* ptr = str;
    while (*ptr != '\r' && *ptr) {
        ptr++;
    }

    if (*ptr == '\r') {
        *ptr == '\0';
    }
}

char* tolower_string(char* str) {
    char* cpy = str;
    for(int i = 0; *(str+i); i++){
        *(str + i) = tolower(*(str + i));
        // printf(" %c ", *(str + i));
    }
    return cpy;
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

    // handle signals
    // signal( SIGTSTP, signal_handler ); // not required; comment out before submitting
    // signal( SIGINT, SIG_IGN );
    // signal( SIGTERM, SIG_IGN );
    // signal( SIGUSR2, SIG_IGN );
    // signal( SIGUSR1, signal_handler );

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

    // reset file pointer
    fseek(file, 0, SEEK_SET);

    int dict_idx = 0;
    while (fgets(word, MAX_LEN, file)) {
        tolower_string(word);
        *(dict+dict_idx) = calloc(MAX_LEN, sizeof(char));
        *(word + stringSize(word)) = '\0';
        strcpy(*(dict+dict_idx), word);
        dict_idx++;
    }

    for (int i = 0; i < num_words; i++) {
        printf("%s, %d\n", dict[i], stringSize(dict[i]));
    }
    // set random seed
    srand( seed );

    char* hidden_word = dict[rand() % num_words];

    // printf("hidden: %s", hidden_word);
    // #=============================================================================
    int server_socks[5] = {0, 0, 0, 0, 0};
    char** usernames = calloc(5, sizeof(char*));

    int					sockfd;
    struct sockaddr_in	servaddr, cliaddr;
    // connect to the server port from STDIN
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Set SO_REUSEADDR option
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Error setting SO_REUSEADDR");
        return 1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(port);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("select error");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 3) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    // printf("waiting on port %d...\n", port);
    socklen_t cli_addr_size;

    fd_set readfds, reads;
    // Initialize the file descriptor set
    FD_ZERO(&reads);
    // FD_SET(STDIN_FILENO, &reads);
    FD_SET(sockfd, &reads); // Add stdin to set

    int num_connected = 0;
    while (1) {
        readfds = reads;

        // Check if any of the connected servers have sent data
        int num_ready = select(9, &readfds, NULL, NULL, NULL);
        if (num_ready < 0) {
            perror("select error");
            exit(1);
        }

        if (num_connected < 5) { 
            // Readline(STDIN_FILENO, port_input, MAXLINE);
            if (FD_ISSET(sockfd, &readfds)) {
                printf("connected!\n");
                int newsockfd = accept(sockfd, (struct sockaddr *) &cliaddr, &cli_addr_size);
                // printf("Connected to port %d\n", serv_port);
                sprintf(msg, "Welcome to Guess the Word, please enter your username.\n");
                int len = sizeof(servaddr);
                // Sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&servaddr, &len);
                send(newsockfd, msg, stringSize(msg), 0);
                // find the empty socket pos
                for (int i = 0; i < 5; i++) {
                    if (server_socks[i] == 0) {
                        server_socks[i] = newsockfd;
                        // server_ports[i] = port;
                        break;
                    }
                }

                // FD_SET(STDIN_FILENO, &reads);
                FD_SET(newsockfd, &reads);
                num_connected++;
            }
        }
        for (int i = 0; i < 5; i++) {
            if (server_socks[i] > 0 && FD_ISSET(server_socks[i], &readfds)) {
                //  at least one server sent a message
                char* buffer = calloc(MAXLINE, sizeof(char));
                int n = recv(server_socks[i], buffer, MAXLINE - 1, 0);
                if (n == 0) {
                    // Server closed connection
                    // printf("Server on %d closed\n", server_ports[i]);
                    FD_CLR(server_socks[i], &reads);
                    close(server_socks[i]);
                    server_socks[i] = 0;
                    free(usernames[i]);
                    usernames[i] = NULL;
                    num_connected--;
                } else if (n > 0) {
                    remove_newline(buffer);
                    buffer[stringSize(buffer)] = '\0';
                        
                    if (usernames[i] == NULL) {
                         // user is setting the username
                        bool duplicate = false;
                        for (int j = 0; j < 5; j++) {
                            if (usernames[j] == NULL) continue;
                            else if (strcmp(tolower_string(usernames[j]), tolower_string(buffer)) == 0) {
                                duplicate = true;
                            }
                        }
                       
                        if (!duplicate) {
                            usernames[i] = buffer;
                            sprintf(msg, "Let's start playing, %s\n", usernames[i]);
                        }
                        else {
                            sprintf(msg, "Username %s is already taken, please enter a different username\n", buffer);
                        }
                        send(server_socks[i], msg, stringSize(msg), 0);                       
                    }
                    else {
                        // user is guessing the word
                        printf("%s \n", buffer);
                    }
                }
            }
        }
    }

    // /* ==================== network setup code above ===================== */
    // running = true;
    // while ( running )
    // {
    //     struct sockaddr_in remote_client;
    //     int addrlen = sizeof( remote_client );

    //     int newsd = accept( listener, (struct sockaddr *)&remote_client,
    //                         (socklen_t *)&addrlen );
    //     // if ( newsd < 0 ) { perror( "accept() failed" ); continue; }

    //     if (!running) break;
    //     if (newsd >= 0) {
    //         printf("MAIN: rcvd incoming connection request\n");
        
    //         /* ==================== application-layer protocol below ============= */
    //         pthread_t thread;
    //         char* hidden_word = calloc(6, sizeof(char)); // this gets freed in main()
    //         int random_idx = rand() % numWords;

    //         strcpy(hidden_word, *(dict + random_idx));

    //         // add this to the array of hidden words...
    //         pthread_mutex_lock( &mutex_on_words ); // MUTEX
    //         words = realloc( words, (words_size + 1 ) * sizeof( char * ) );
    //         toupper_string(hidden_word);
    //         *(words + words_size - 1) = hidden_word;
    //         *(words + words_size) = NULL;
    //         words_size++;
    //         pthread_mutex_unlock( &mutex_on_words ); // MUTEX ENDS
    //         // printf("hidden_word: %s\nactual word: %s, size: %d\n idx: %d\n", hidden_word, *(dict + random_idx), (int) strlen(hidden_word), random_idx);

    //         // pass these as arguments into client_guesss
    //         arguments args;
    //         args.newsd = newsd;
    //         args.hidden_word = hidden_word;
    //         args.dict = dict;
    //         args.numWords = numWords;
            
    //         // create a thread... (continues in client_guess())
    //         pthread_create( &thread, NULL, client_guess, &args );
            
    //         // free up memory
    //         // free(hidden_word);
    //     }
        
    // }


    // close( listener );

    for (int i = 0; i < 5; i++) {
        free(*(usernames + i));
    }
    free(word);
    for (int i = 0; i < num_words; i++) {
        free(*(dict + i));
    }
    free(dict);
    fclose(file);

    return EXIT_SUCCESS;
}