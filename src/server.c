#include "utils.h"

#define PORT 8080

Client clients[MAX_CLIENTS];
int client_count = 0;

void broadcast_message(Message *msg, int sender_sd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket != sender_sd) {
            send(clients[i].socket, msg, sizeof(Message), 0);
        }
    }
}

void handle_client_message(int sd) {
    Message msg;
    int received = recv(sd, &msg, sizeof(Message), 0);
    
    if (received <= 0) {
        // Handle disconnection
        for (int i = 0; i < client_count; i++) {
            if (clients[i].socket == sd) {
                Message leave_msg;
                leave_msg.type = MSG_LEAVE;
                strncpy(leave_msg.sender, clients[i].name, NAME_LEN-1);
                snprintf(leave_msg.content, BUFFER_SIZE, "%s has left the chat", clients[i].name);
                
                // Remove client
                while (i < client_count - 1) {
                    clients[i] = clients[i + 1];
                    i++;
                }
                client_count--;
                broadcast_message(&leave_msg, sd);
                close(sd);
                break;
            }
        }
    } else {
        broadcast_message(&msg, sd);
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    fd_set master_fds, read_fds;
    int max_fd;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Initialize fd sets
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_fd = server_fd;

    printf("Server is running on port %d\n", PORT);

    while (1) {
        read_fds = master_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == server_fd) {
                    // New connection
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
                    
                    if (new_socket < 0) {
                        perror("Accept failed");
                        continue;
                    }

                    FD_SET(new_socket, &master_fds);
                    if (new_socket > max_fd) {
                        max_fd = new_socket;
                    }

                    // Add to clients array
                    clients[client_count].socket = new_socket;
                    recv(new_socket, clients[client_count].name, NAME_LEN, 0);

                    // Send join message
                    Message join_msg;
                    join_msg.type = MSG_JOIN;
                    strncpy(join_msg.sender, clients[client_count].name, NAME_LEN-1);
                    snprintf(join_msg.content, BUFFER_SIZE, "%s has joined the chat", clients[client_count].name);
                    broadcast_message(&join_msg, new_socket);
                    
                    client_count++;
                } else {
                    // Handle client message
                    handle_client_message(fd);
                }
            }
        }
    }

    return 0;
}
