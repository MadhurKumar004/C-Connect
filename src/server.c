#include "utils.h"
#include <sys/select.h>
#include <errno.h>

#define PORT 8080

Client clients[MAX_CLIENTS];
int client_count = 0;
fd_set master_fds, read_fds;
int max_fd;

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
        char client_name[NAME_LEN] = "Unknown";
        for (int i = 0; i < client_count; i++) {
            if (clients[i].socket == sd) {
                strncpy(client_name, clients[i].name, NAME_LEN-1);
                
                // Send disconnect message
                Message leave_msg;
                leave_msg.type = MSG_LEAVE;
                strncpy(leave_msg.sender, "SERVER", NAME_LEN-1);
                snprintf(leave_msg.content, BUFFER_SIZE, "%s has left the chat", client_name);
                
                // Remove client
                while (i < client_count - 1) {
                    clients[i] = clients[i + 1];
                    i++;
                }
                client_count--;
                
                // Broadcast before closing
                broadcast_message(&leave_msg, sd);
                FD_CLR(sd, &master_fds);
                close(sd);
                printf("Client %s disconnected\n", client_name);
                break;
            }
        }
    } else if (received == sizeof(Message)) {
        broadcast_message(&msg, sd);
    }
}

void print_server_ip() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) return;

    // Using Google's DNS server as a dummy destination
    struct sockaddr_in dummy_addr;
    memset(&dummy_addr, 0, sizeof(dummy_addr));
    dummy_addr.sin_family = AF_INET;
    dummy_addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    dummy_addr.sin_port = htons(53);

    // Try to connect (won't actually send anything)
    if (connect(sock, (struct sockaddr*)&dummy_addr, sizeof(dummy_addr)) == -1) {
        close(sock);
        return;
    }

    // Get local address information
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(sock, (struct sockaddr*)&local_addr, &addr_len) == -1) {
        close(sock);
        return;
    }

    printf("Server IP address: %s\n", inet_ntoa(local_addr.sin_addr));
    close(sock);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if((server_fd=socket(AF_INET, SOCK_STREAM, 0))==0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_fd = server_fd;

    printf("Server started on port %d\n", PORT);
    print_server_ip();
    printf("\nUse one of these IP addresses to connect from other devices on the network\n");

    while (1) {
        read_fds = master_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;  // Handle interrupt
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == server_fd) {
                    struct sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
                    
                    if (client_socket < 0) {
                        perror("accept failed");
                        continue;
                    }

                    if (client_count >= MAX_CLIENTS) {
                        printf("Maximum clients reached. Connection rejected.\n");
                        close(client_socket);
                        continue;
                    }

                    FD_SET(client_socket, &master_fds);
                    if (client_socket > max_fd) {
                        max_fd = client_socket;
                    }

                    clients[client_count].socket = client_socket;
                    char name_buf[NAME_LEN];
                    int name_len = recv(client_socket, name_buf, NAME_LEN-1, 0);
                    if (name_len > 0) {
                        name_buf[name_len] = '\0';
                        strncpy(clients[client_count].name, name_buf, NAME_LEN-1);
                        clients[client_count].name[NAME_LEN-1] = '\0';
                        
                        Message join_msg;
                        join_msg.type = MSG_JOIN;
                        strncpy(join_msg.sender, "SERVER", NAME_LEN-1);
                        snprintf(join_msg.content, BUFFER_SIZE, "%s has joined the chat", clients[client_count].name);
                        broadcast_message(&join_msg, client_socket);
                        
                        printf("New client: %s\n", clients[client_count].name);
                        client_count++;
                    } else {
                        close(client_socket);
                        FD_CLR(client_socket, &master_fds);
                    }
                } else {
                    handle_client_message(fd);
                }
            }
        }
    }
    return 0;
}
