#include "utils.h"
#include <time.h>

#define PORT 8080

volatile int running = 1;  // Global flag for thread control

void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

void register_user(Client *client) {
    const char *colors[] = {RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN};
    int num_colors = sizeof(colors) / sizeof(colors[0]);
    printf("Enter your name: ");
    fgets(client->name, NAME_LEN, stdin);
    client->name[strcspn(client->name, "\n")] = 0; 

    srand(time(NULL));
    const char *random_color = colors[rand() % num_colors];
    snprintf(client->color, sizeof(client->color), "%s", random_color);
}

void print_help() {
    printf("Commands:\n");
    printf("-help: Show this help message\n");
    printf("-register: Register a new user\n");
    printf("To send a public message, just type and press enter.\n");
    printf("To send a private message, use @username: message\n");
}

void *receive_messages(void *arg) {
    int socket = *(int *)arg;
    Message msg;
    
    while (running) {
        int read_len = recv(socket, &msg, sizeof(Message), 0);
        if (read_len <= 0) {
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
    signal(SIGINT, handle_signal);  // Handle Ctrl+C

    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return -1;
    }

    int client_socket;
    struct sockaddr_in server_addr;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    Client client;
    client.socket = client_socket;
    register_user(&client);

    // Send name to server
    send(client_socket, client.name, strlen(client.name), 0);

    // Create receive thread
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_messages, &client_socket) != 0) {
        perror("Failed to create receiver thread");
        close(client_socket);
        return 1;
    }

    // Main message loop
    char buffer[BUFFER_SIZE];
    while (running) {
        printf("> ");
        fflush(stdout);
        
        if (!fgets(buffer, BUFFER_SIZE, stdin) || !running) {
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0;

        Message msg;
        msg.type = PUBLIC_MESSAGE;
        strncpy(msg.sender, client.name, NAME_LEN-1);
        msg.sender[NAME_LEN-1] = '\0';
        strncpy(msg.content, buffer, BUFFER_SIZE-1);
        msg.content[BUFFER_SIZE-1] = '\0';
        
        if (send(client_socket, &msg, sizeof(Message), 0) < 0) {
            perror("Send failed");
            break;
        }
    }

    // Cleanup
    close(client_socket);
    running = 0;
    sleep(1);  // Give receive thread time to exit
    return 0;
}
