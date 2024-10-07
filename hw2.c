#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include "../../lib/unp.h"
// #include "unp.h" 

int stringSize(char *str) {
    int length = 0;
    char* ptr = str;
    // Loop till the NULL character is found
    while (*ptr != '\0' && *ptr != '\n' && *ptr != '\r' && *ptr) {
        length++;
        ptr++;
    }
    return length;
}

void tolower_string(char* str) {
    printf("%s", str);
    for(int i = 0; *(str+i); i++){
        *(str + i) = tolower(*(str + i));
        // printf(" %c ", *(str + i));
    }
    // printf("\n");
}

int main(int argc, char ** argv ) {
    if (argc != 5) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: ./word_guess.out [seed] [port] [dictionary_file] [longest_word_length]\n");
        return EXIT_FAILURE;
    }

    int port = atoi(*(argv+1));
    int seed = atoi(*(argv+2));
    char* filepath = *(argv+3);
    int MAX_LEN = atoi(*(argv+4)) + 1;
    
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

    char* test = "test";
    printf("len: %d \n", stringSize(test));
    // save all valid words
    char** dict = calloc(num_words, sizeof(char*));

    printf("%d \n", num_words);

    // reset file pointer
    fseek(file, 0, SEEK_SET);

    int dict_idx = 0;
    while (fgets(word, MAX_LEN, file)) {
        // if (strlen(word) != 5) continue; 
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

    // #=============================================================================
    // int server_socks[5] = {0, 0, 0, 0, 0};
    // int server_ports[5] = {0, 0, 0, 0, 0};

    // fd_set		readfds, reads;

    // // Initialize the file descriptor set
    // FD_ZERO(&readfds);
    // FD_SET(STDIN_FILENO, &reads); // Add stdin to set

    // while (1) {
    //     readfds = reads;

    //     // Check if any of the connected servers have sent data
    //     int num_ready = select(8, &readfds, NULL, NULL, NULL);
    //     if (num_ready < 0) {
    //         perror("select error");
    //         exit(1);
    //     }

    //     if (num_connected < 5 && FD_ISSET(STDIN_FILENO, &readfds)) { // 
    //         int					sockfd;
    //         struct sockaddr_in	servaddr;
    //         int serv_port;
    //         Readline(STDIN_FILENO, port_input, MAXLINE);
    //         serv_port = atoi(port_input);

    //         // connect to the server port from STDIN
    //         sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    //         bzero(&servaddr, sizeof(servaddr));
    //         servaddr.sin_family      = AF_INET;
    //         servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //         servaddr.sin_port        = htons(serv_port);

    //         if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
    //             // printf("Failed to connect to port %d\n", serv_port);
    //             close(sockfd);
    //         } else {
    //             // printf("Connected to port %d\n", serv_port);
    //             // find the empty socket pos
    //             for (int i = 0; i < 5; i++) {
    //                 if (server_socks[i] == 0) {
    //                     server_socks[i] = sockfd;
    //                     server_ports[i] = serv_port;
    //                     break;
    //                 }
    //             }

    //             FD_SET(STDIN_FILENO, &reads);
    //             FD_SET(sockfd, &reads);
    //             num_connected++;
    //         }
    //     }

        
    //     for (int i = 0; i < 5; i++) {
    //         if (server_socks[i] > 0 && FD_ISSET(server_socks[i], &readfds)) {
    //             //  at least one server sent a message
    //             int n = recv(server_socks[i], buffer, MAXLINE - 1, 0);
    //             if (n == 0) {
    //                 // Server closed connection
    //                 printf("Server on %d closed\n", server_ports[i]);
    //                 FD_CLR(server_socks[i], &reads);
    //                 close(server_socks[i]);
    //                 server_socks[i] = 0;
    //                 server_ports[i] = 0;
    //                 num_connected--;
    //             } else if (n > 0) {
    //                 buffer[n] = '\0';
    //                 printf("%d %s", server_ports[i], buffer);
    //             }
    //         }
    //     }
    // }

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

    free(word);
    for (int i = 0; i < num_words; i++) {
        free(*(dict + i));
    }
    free(dict);
    fclose(file);

    return EXIT_SUCCESS;
}