#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>

extern int total_guesses;
extern int total_wins;
extern int total_losses;
extern char ** words;
int words_size;
bool running;   // decides when to shut down the server
int listener;

/* global mutex variables */
pthread_mutex_t mutex_on_total_guesses = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_on_total_wins = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_on_total_losses = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_on_words = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t mutex_on_running = PTHREAD_MUTEX_INITIALIZER;

/*
netcat -u linux02.cs.rpi.edu 41234
          ^^^^^^
          replace with your hostname
*/

typedef struct s_arguments {
    int newsd;
    char* hidden_word;
    fd_set readfds;
    char** dict;
    int numWords;
} arguments;

bool isnumber(char* str) {
    int length = strlen(str);
    for (int i = 0; i < length; i++) {
        if (!isdigit(*(str + i))) return false;
    }
    return true;
}

void signal_handler( int sig ) {
    if ( sig == SIGUSR1 ) {
        printf( "MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n" );
        // pthread_mutex_lock(&mutex_on_running);
        running = false;
        // pthread_mutex_unlock(&mutex_on_running);
        close(listener);
        
    }
    else if ( sig == SIGTSTP) { // comment this out before submitting
        signal( SIGUSR1, signal_handler );
        // printf(" shutting down the server with SIGSTP...\n");
        pid_t pid = getpid();
        kill(pid, SIGUSR1);
    }
}

void toupper_string(char* str) {
    for(int i = 0; *(str+i); i++){
        *(str + i) = toupper(*(str + i));
    }
}

void tolower_string(char* str) {
    for(int i = 0; *(str+i); i++){
        *(str + i) = tolower(*(str + i));
    }
}

// ignore case
bool strings_match(char* word, char* word2) {
    for (int i = 0; i < 5; i++) {
        if (tolower(*(word + i)) != tolower(*(word2 + i))) return false;
    }
    return true;
}

// if word is in the dictionary, return true
bool is_valid_word(char* guess, char** dict, int numWords) {
    for (int i = 0; i < numWords; i++) {
        if (strings_match(*(dict + i), guess)) return true;
    }
    return false;
}

int count_char(char* guess) {
    int count = 0;
    for (int i = 0; i < 5; i++) {
        if (isalpha(*(guess + i))) count++;
    }
    return count;
}

// returns the number of characters
int shift_string(char* guess) {
    int idx = 0;
    for (int i = 0; i < 5; i++) {
        if(isalpha(*(guess + i))) {
            *(guess + idx) = *(guess + i);
            idx++;
        }
    }
    return idx;
}

void reset_buffer(char* buffer) {
    for (int i = 0; i < 5; i++) {
        *(buffer + i) = '\0';
    }
}


// hidden_word is actually a copy of the original hidden_word string
char* result_string(char* hidden_word, char* guess) {
    char* result = calloc(6, sizeof(char)); // will be freed in client_guess

    // first get all the matching characters
    for (int i = 0; i < 5; i++) {
        if (tolower(*(hidden_word + i)) == tolower(*(guess + i))) {
            *(result + i) = toupper(*(guess + i));
            *(hidden_word + i) = '.';         // since this character was "used", delete it
        }
    }

    // then get all the characters that are in the hidden word, just out of order
    for (int i = 0; i < 5; i++) {
        if (*(result + i) == '\0') {
            bool match_found = false;
            for (int j = 0; j < 5; j++) {
                if (tolower(*(hidden_word + j)) == tolower(*(guess + i))) { // this character is used in hidden word
                    *(result + i) = tolower(*(guess + i));
                    *(hidden_word + j) = '.'; // since this character was "used", delete it
                    match_found = true;
                    break;
                }
            }
            if (!match_found) {
                *(result + i) = '-'; // this character is not used in any of the hidden word
            }
        }
    }
    return result;
}

void* client_guess( void * arg ) {
    pthread_t tid = pthread_self();  /* get my thread ID */

    /* pthread_detach() enables pthreads to reclaim the
    *  thread resources when this thread terminates,
    *   i.e., do not leave a "zombie" thread behind
    */
    pthread_detach( tid ); 

    arguments args = *(arguments *) arg;
    int newsd = args.newsd;
    char* hidden_word = args.hidden_word;

    char** dict = args.dict;
    int numWords = args.numWords;

    short guesses_left = 6;
    bool word_found = false;
    char* guess = calloc(6, sizeof(char));
    char* return_pack = calloc(9, sizeof(char));
    // char* guess_buffer = calloc(1, sizeof(char));
    // int idx = 0;

    while (guesses_left > 0 && !word_found && running) {
        // NON-BLOCKING UNTIL INPUT
        printf("THREAD %lu: waiting for guess\n", tid);
        // int n = recv( newsd, guess, 5, 0 ); // These calls return the number of bytes received, or -1 if an error occurred.  In the event of an error, errno is set to indicate the error.

        reset_buffer(guess);
        int n = 0;
        bool gave_up = false;
        while (n < 5) {
            n = recv( newsd, guess + n, 5-n, 0 );
            if (n == -1 || n == 0) {
                gave_up = true;
                break;
            }
            n = shift_string(guess); // n = the number of valid characters (passes isalpha())
        }
        if (gave_up) {
            printf( "THREAD %lu: client gave up; closing TCP connection...\n", tid );
            break;
        }

        // if (!running) break;

        if (n != 5) {
            fprintf(stderr, "ERROR: n is %d", n);
        }
        else if ( n == 5 && is_valid_word(guess, dict, numWords)) {
            // idx = 0;
            guesses_left--;

            pthread_mutex_lock( &mutex_on_total_guesses );
            total_guesses++;   /* CRITICAL SECTION */
            pthread_mutex_unlock( &mutex_on_total_guesses );
            

            printf("THREAD %lu: rcvd guess: %s\n", tid, guess);


            if (strings_match(hidden_word, guess) == true) { // user guessed correctly
                word_found = true;
            }
            
            *(return_pack) = 'Y';
            char* hidden_word_copy = calloc(6, sizeof(char));
            strcpy(hidden_word_copy, hidden_word); // make a copy of the answer string for manipulation in result_string()
            char* result = result_string(hidden_word_copy, guess);

            short converted_guesses = htons(guesses_left);
            memcpy(return_pack + 1, &converted_guesses, sizeof(short));
            strcpy(return_pack+3, result);
            n = send( newsd, return_pack, sizeof( return_pack ), 0);
            
            if ( n == -1 || n == 0 ) { // client terminated
                printf( "THREAD %lu: client gave up; closing TCP connection...\n", tid );
                free(hidden_word_copy);
                free(result);
                break;
            }
            
            if (guesses_left == 1) printf("THREAD %lu: sending reply: %s (1 guess left)\n", tid, result);
            else printf("THREAD %lu: sending reply: %s (%d guesses left)\n", tid, result, guesses_left);
            free(hidden_word_copy);
            free(result);
        }
        else { // if the word is 5 characters long (just not valid )
            printf("THREAD %lu: rcvd guess: %s\n", tid, guess);
            *return_pack = 'N';
            short converted_guesses = htons(guesses_left);
            memcpy(return_pack + 1, &converted_guesses, sizeof(short));
            strcpy(return_pack+3, "?????");
            n = send( newsd, return_pack, sizeof( return_pack ), 0);
            if ( n == -1 || n == 0 ) { // client terminated
                printf( "THREAD %lu: client gave up; closing TCP connection...\n", tid);
                break;
            }
            printf("THREAD %lu: invalid guess; sending reply: ????? (%d guesses left)\n", tid, guesses_left);
        }
        // }
    }
    
    if (!running) { // if closed  abruptly
        free(guess);
        free(return_pack);
        close( newsd );
        return NULL;
    }

    toupper_string(hidden_word);
    printf("THREAD %lu: game over; word was %s!\n", tid, hidden_word);
    
    // MUTEX
    if (word_found) {
        pthread_mutex_lock( &mutex_on_total_wins );
        total_wins++;   /* CRITICAL SECTION */
        pthread_mutex_unlock( &mutex_on_total_wins );
    }
    else {
        pthread_mutex_lock( &mutex_on_total_losses );
        total_losses++;   /* CRITICAL SECTION */
        pthread_mutex_unlock( &mutex_on_total_losses );
    }

    free(guess);
    free(return_pack);
    close( newsd ); 
    return NULL;
}

// void remove_newline( char* str ) {
//     char* tmp = str;
//     for (int i = 0; i < strlen(str); i++) {
//         if (*(tmp + i) == '\n' && i + 1 < strlen(str)) (tmp + i) = ()
//     }
// }
int wordle_server( int argc, char ** argv )
{
    if (argc != 5) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw3.out <listener-port> <seed> <dictionary-filename> <num-words>\n");
        return EXIT_FAILURE;
    }

    char* input_str1 = *(argv + 1);
    char* input_str2 = *(argv + 2);
    char* input_str3 = *(argv + 4);

    if (!isnumber(input_str1) || !isnumber(input_str2) || !isnumber(input_str3)) {
        fprintf(stderr, "ERROR: one of the arguments is not an integer!\n");
        return EXIT_FAILURE;
    }

    int port = atoi(*(argv+1));
    int seed = atoi(*(argv+2));
    char* filepath = *(argv+3);
    int numWords = atoi(*(argv+4));
    words_size = 1;
    

    // handle signals
    signal( SIGTSTP, signal_handler ); // not required; comment out before submitting
    signal( SIGINT, SIG_IGN );
    signal( SIGTERM, SIG_IGN );
    signal( SIGUSR2, SIG_IGN );
    signal( SIGUSR1, signal_handler );

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: INVALID FILE PATH\n");
        return EXIT_FAILURE;
    }

    // save all valid words
    char* word = calloc(6, sizeof(char));
    char** dict = calloc(numWords, sizeof(char*));

    int i = 0;
    // int idx = 0;
    // char* char_buffer = calloc(1, sizeof(char));
    // while (read(file, char_buffer, 1) > 0) {
    //     if (isalpha(*char_buffer)) {
    //         *(word + idx) = *char_buffer;
    //     }
    //     else { // delimiter found
    //         if (strlen(word) == 5) {
    //             *(word + idx) = '\0';
    //             *(dict+i) = calloc(6, sizeof(char)); 
    //             printf("word: %s\n", word);
    //             strcpy(*(dict+i), word);
    //             i++;
    //             idx = -1;  //reset index
    //         }
    //     }
    //     idx++;
    // }

    while (fgets(word, 6, file)) {
        if (strlen(word) != 5) continue; 
        tolower_string(word);
        *(dict+i) = calloc(6, sizeof(char));
        *(word + 5) = '\0';
        strcpy(*(dict+i), word);
        i++;
    }


    srand( seed );

    // #=============================================================================

    /* Create the listener socket as TCP socket (SOCK_STREAM) */
    listener = socket( AF_INET, SOCK_STREAM, 0 );
                                /* here, SOCK_STREAM indicates TCP */

    if ( listener == -1 ) { 
        fprintf(stderr, "ERROR: socket() failed" ); 
        return EXIT_FAILURE; 
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "ERROR: SO_REUSEADDR FAILED");
    }

    // Set the socket to non-blocking mode
    // int flags = fcntl(listener, F_GETFL, 0);
    // fcntl(listener, F_SETFL, flags | O_NONBLOCK);

    /* populate the socket structure for bind() */
    struct sockaddr_in tcp_server;
    tcp_server.sin_family = AF_INET;   /* IPv4 */

    tcp_server.sin_addr.s_addr = htonl( INADDR_ANY );
    /* allow any remote IP address to connect to our socket */

    tcp_server.sin_port = htons( port );

    if ( bind( listener, (struct sockaddr *)&tcp_server, sizeof( tcp_server ) ) == -1 )
    {
        fprintf(stderr, "ERROR: bind() failed" );
        return EXIT_FAILURE;
    }

    /* identify our port number as a TCP listener port */
    if ( listen( listener, 5 ) == -1 )
    {
        fprintf(stderr, "ERROR: listen() failed" );
        return EXIT_FAILURE;
    }

    if (numWords == 1) printf("MAIN: opened %s (1 word)\n", filepath);
    else printf("MAIN: opened %s (%d words)\n", filepath, numWords);

    printf("MAIN: seeded pseudo-random number generator with %d\n", seed);
    printf("MAIN: Wordle server listening on port {%d}\n", port);

    /* ==================== network setup code above ===================== */
    running = true;
    while ( running )
    {
        struct sockaddr_in remote_client;
        int addrlen = sizeof( remote_client );

        int newsd = accept( listener, (struct sockaddr *)&remote_client,
                            (socklen_t *)&addrlen );
        // if ( newsd < 0 ) { perror( "accept() failed" ); continue; }

        if (!running) break;
        if (newsd >= 0) {
            printf("MAIN: rcvd incoming connection request\n");
        
            /* ==================== application-layer protocol below ============= */
            pthread_t thread;
            char* hidden_word = calloc(6, sizeof(char)); // this gets freed in main()
            int random_idx = rand() % numWords;

            strcpy(hidden_word, *(dict + random_idx));

            // add this to the array of hidden words...
            pthread_mutex_lock( &mutex_on_words ); // MUTEX
            words = realloc( words, (words_size + 1 ) * sizeof( char * ) );
            toupper_string(hidden_word);
            *(words + words_size - 1) = hidden_word;
            *(words + words_size) = NULL;
            words_size++;
            pthread_mutex_unlock( &mutex_on_words ); // MUTEX ENDS
            // printf("hidden_word: %s\nactual word: %s, size: %d\n idx: %d\n", hidden_word, *(dict + random_idx), (int) strlen(hidden_word), random_idx);

            // pass these as arguments into client_guesss
            arguments args;
            args.newsd = newsd;
            args.hidden_word = hidden_word;
            args.dict = dict;
            args.numWords = numWords;
            
            // create a thread... (continues in client_guess())
            pthread_create( &thread, NULL, client_guess, &args );
            
            // free up memory
            // free(hidden_word);
        }
        
    }


    // close( listener );

    free(word);
    for (int i = 0; i < numWords; i++) {
        free(*(dict + i));
    }
    free(dict);
    fclose(file);

    return EXIT_SUCCESS;
}

// BASE
// int main( int argc, char ** argv )
// {
//   total_guesses = total_wins = total_losses = 0;
//   words = calloc( 1, sizeof( char * ) );
//   if ( words == NULL ) { perror( "calloc() failed" ); return EXIT_FAILURE; }

//   int rc = wordle_server( argc, argv );

//   /* on Submitty, there will be more code here that validates
//    *  the global variables at the end of your code...
//    */

//   /* deallocate memory for the list of words played */
//   for ( char ** ptr = words ; *ptr ; ptr++ )
//   {
//     free( *ptr );
//   }
//   free( words );
//   return rc;
// }