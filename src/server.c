#define _POSIX_C_SOURCE 200112L 
#include "utils.h"
#include <ifaddrs.h>
#include <netdb.h>

#define PORT 8080

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex=PTHREAD_MUTEX_INITIALIZER;
int client_count=0;

void *handle_client(void *arg){
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];
    Message msg;

    snprintf(buffer, BUFFER_SIZE, "%s had joined the chat!", client->name);
    printf("%s\n",buffer);

    while(1){
        int read_len=recv(client->socket,&msg,sizeof(Message),0);
        if(read_len==0){
            break;
        }

        pthread_mutex_lock(&clients_mutex);
        switch(msg.type){
            case PUBLIC_MESSAGE:
                for(int i=0;i<client_count;i++){
                    if(clients[i].socket!=client->socket){
                        send(clients[i].socket,&msg,sizeof(Message),0);
                    }
                }
                break;
            
            case PRIVATE_MESSAGE:
                for(int i=0;i<client_count;i++){
                    if(strcmp(clients[i].name,msg.receiver)==0){
                        send(clients[i].socket,&msg,sizeof(Message),0);
                        break;
                    }
                }
                break;
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    pthread_mutex_lock(&clients_mutex);
    for(int i=0;i<client_count;i++){
        if(clients[i].socket==client->socket){
            while(i<client_count-1){
                clients[i]=clients[i+1];
                i++;
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(client->socket);
    snprintf(buffer, BUFFER_SIZE, "%s had left the chat!", client->name);
    printf("%s\n",buffer);

    return NULL;
}

void print_server_ip() {
    char host[256];
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    printf("Server IP addresses:\n");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
            continue;

        getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                   host, sizeof(host),
                   NULL, 0, NI_NUMERICHOST);
                   
        if (strcmp(ifa->ifa_name, "lo") != 0) {  // Skip localhost
            printf("%s: %s\n", ifa->ifa_name, host);
        }
    }
    
    freeifaddrs(ifaddr);
}

int main(){
    int server_fd;
    struct sockaddr_in address;
    int opt=1;

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
    
    printf("Server started on port %d\n", PORT);
    print_server_ip();
    printf("\nUse one of these IP addresses to connect from other devices on the network\n");

    while(1){
        struct sockaddr_in client_addr;
        int client_socket;
        socklen_t addrlen=sizeof(client_addr);

        if((client_socket=accept(server_fd, (struct sockaddr *)&client_addr, &addrlen))<0){
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        pthread_t thread_id;
        Client *client=&clients[client_count];
        client->socket=client_socket;

        // Initialize client name
        char name_buf[NAME_LEN];
        int name_len = recv(client_socket, name_buf, NAME_LEN-1, 0);
        if (name_len <= 0) {
            perror("Failed to receive client name");
            close(client_socket);
            continue;
        }
        name_buf[name_len] = '\0';
        strcpy(client->name, name_buf);

        // Create and broadcast join message
        Message join_msg;
        join_msg.type = PUBLIC_MESSAGE;
        strncpy(join_msg.sender, "SERVER", NAME_LEN-1);
        snprintf(join_msg.content, BUFFER_SIZE, "%s has joined the chat!", client->name);
        
        pthread_mutex_lock(&clients_mutex);
        client_count++;
        // Broadcast to existing clients
        for(int i = 0; i < client_count-1; i++) {
            send(clients[i].socket, &join_msg, sizeof(Message), 0);
        }
        pthread_mutex_unlock(&clients_mutex);

        pthread_create(&thread_id, NULL, handle_client, (void *)client);
        pthread_detach(thread_id);
    }
    
    return 0;
}
