#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 8080
#define MAX_PENDING_CONNECTIONS 5
#define NUM_CHILDREN 5 // Số lượng tiến trình con được tạo trước

void handle_client(int client_socket) {
    char buf[1024];
    ssize_t ret;

    printf("New client connected: %d\n", client_socket);

    // Receive data from client and print to stdout
    ret = recv(client_socket, buf, sizeof(buf), 0);
    buf[ret] = '\0';
    puts(buf);

    // Send response to client
    char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
    send(client_socket, msg, strlen(msg), 0);

    // Close connection
    close(client_socket);
}

int main() {
    int listener, client;
    struct sockaddr_in server_addr;

    // Create socket
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        close(listener);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listener, MAX_PENDING_CONNECTIONS) == -1) {
        perror("Error listening on socket");
        close(listener);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    // Preforking: Create a pool of child processes
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error forking process");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Child process
            while (1) {
                // Accept new connection
                client = accept(listener, NULL, NULL);
                if (client == -1) {
                    perror("Error accepting connection");
                    continue;
                }

                // Handle client request
                handle_client(client);
            }
        }
    }

    // Parent process just waits for children to exit
    while (waitpid(-1, NULL, 0) > 0);

    close(listener);
    return 0;
}
