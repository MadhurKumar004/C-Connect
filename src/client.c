#include "utils.h"
#include <signal.h>

#define PORT 8080

volatile sig_atomic_t running = 1;

void *receive_messages(void *arg) {
    int socket = *(int *)arg;
    Message msg;
    
    while (running) {
        int received = recv(socket, &msg, sizeof(Message), 0);
        if (received <= 0) {
            printf("\nDisconnected from server\n");
            running = 0;
            break;
        }
        printf("\r%s: %s\n> ", msg.sender, msg.content);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    int client_socket;
    struct sockaddr_in server_addr;
    char name[NAME_LEN];

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return 1;
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    // Get username
    printf("Enter your name: ");
    fgets(name, NAME_LEN, stdin);
    name[strcspn(name, "\n")] = 0;
    send(client_socket, name, strlen(name), 0);

    // Create receive thread
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, &client_socket);

    // Main message loop
    char buffer[BUFFER_SIZE];
    while (running) {
        printf("> ");
        fflush(stdout);
        
        if (!fgets(buffer, BUFFER_SIZE, stdin)) break;
        buffer[strcspn(buffer, "\n")] = 0;

        Message msg;
        msg.type = MSG_PUBLIC;
        strncpy(msg.sender, name, NAME_LEN-1);
        strncpy(msg.content, buffer, BUFFER_SIZE-1);
        
        if (send(client_socket, &msg, sizeof(Message), 0) < 0) {
            perror("Send failed");
            break;
        }
    }

    close(client_socket);
    return 0;
}
