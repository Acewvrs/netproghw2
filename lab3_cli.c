#include <stdio.h>
#include "../../lib/unp.h"
// #include "unp.h" 

pid_t pid_global;
int child_terminated = 0;

int main() {
    int server_socks[5] = {0, 0, 0, 0, 0};
    int server_ports[5] = {0, 0, 0, 0, 0};
    char buffer[MAXLINE];    
	fd_set		readfds, reads;

    // Initialize the file descriptor set
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &reads); // Add stdin to set
    
    char port_input[MAXLINE + 1];
    int num_connected = 0;
    while (1) {
        readfds = reads;

        // Check if any of the connected servers have sent data
        int num_ready = select(8, &readfds, NULL, NULL, NULL);
        if (num_ready < 0) {
            perror("select error");
            exit(1);
        }

        if (num_connected < 5 && FD_ISSET(STDIN_FILENO, &readfds)) { // 
            int					sockfd;
            struct sockaddr_in	servaddr;
            int serv_port;
            Readline(STDIN_FILENO, port_input, MAXLINE);
            serv_port = atoi(port_input);

            // connect to the server port from STDIN
            sockfd = Socket(AF_INET, SOCK_STREAM, 0);

            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family      = AF_INET;
            servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            servaddr.sin_port        = htons(serv_port);

            if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
                // printf("Failed to connect to port %d\n", serv_port);
                close(sockfd);
            } else {
                // printf("Connected to port %d\n", serv_port);
                // find the empty socket pos
                for (int i = 0; i < 5; i++) {
                    if (server_socks[i] == 0) {
                        server_socks[i] = sockfd;
                        server_ports[i] = serv_port;
                        break;
                    }
                }

                FD_SET(STDIN_FILENO, &reads);
                FD_SET(sockfd, &reads);
                num_connected++;
            }
        }

        
        for (int i = 0; i < 5; i++) {
            if (server_socks[i] > 0 && FD_ISSET(server_socks[i], &readfds)) {
                //  at least one server sent a message
                int n = recv(server_socks[i], buffer, MAXLINE - 1, 0);
                if (n == 0) {
                    // Server closed connection
                    printf("Server on %d closed\n", server_ports[i]);
                    FD_CLR(server_socks[i], &reads);
                    close(server_socks[i]);
                    server_socks[i] = 0;
                    server_ports[i] = 0;
                    num_connected--;
                } else if (n > 0) {
                    buffer[n] = '\0';
                    printf("%d %s", server_ports[i], buffer);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
