#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>

#define PORT 9000
#define MAX_PENDING_CONNECTIONS 5
#define BUFFER_SIZE 1024

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    char tmp[10];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received data
        // Check if the received command contains "GET_TIME"
        if (strncmp(buffer, "GET_TIME", 8) == 0) {
            char format[BUFFER_SIZE];
            // Extract the format from the command using sscanf
            if (sscanf(buffer, "%*s %s %s", format, tmp) == 1) {
                // Get the current time in the requested format
                time_t raw_time;
                struct tm *timeinfo;
                time(&raw_time);
                timeinfo = localtime(&raw_time);
                char time_str[BUFFER_SIZE];
                if (strcmp(format, "dd/mm/yyyy") == 0) {
                    strftime(time_str, BUFFER_SIZE, "%d/%m/%Y", timeinfo);
                } else if (strcmp(format, "dd/mm/yy") == 0) {
                    strftime(time_str, BUFFER_SIZE, "%d/%m/%y", timeinfo);
                } else if (strcmp(format, "mm/dd/yyyy") == 0) {
                    strftime(time_str, BUFFER_SIZE, "%m/%d/%Y", timeinfo);
                } else if (strcmp(format, "mm/dd/yy") == 0) {
                    strftime(time_str, BUFFER_SIZE, "%m/%d/%y", timeinfo);
                } else {
                    strcpy(time_str, "Invalid format");
                }
                // Send the formatted time back to the client
                send(client_socket, time_str, strlen(time_str), 0);
            } else {
                send(client_socket, "Invalid format", 15, 0);
            }
        } else {
            send(client_socket, "Invalid command", 15, 0);
        }
    }

    close(client_socket);
}



int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pid_t child_pid;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind 
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_socket, MAX_PENDING_CONNECTIONS) == -1) {
        perror("listen() failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    // Accept
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("accept() failed");
            continue;
        }

        // Fork a new process to handle the client connection
        child_pid = fork();
        if (child_pid == -1) {
            perror("fork() failed");
            close(client_socket);
            continue;
        } else if (child_pid == 0) { // Child process
            close(server_socket); // Close server socket in child process
            handle_client(client_socket); // Handle client connection
            exit(EXIT_SUCCESS);
        } else { // Parent process
            close(client_socket); // Close client socket in parent process
            // Wait for the child process to exit to prevent zombie processes
            while (waitpid(-1, NULL, WNOHANG) > 0);
        }
    }

    close(server_socket);
    return 0;
}
