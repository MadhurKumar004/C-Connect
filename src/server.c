 #include "utils.h"

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
