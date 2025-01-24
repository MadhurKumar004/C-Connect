#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>      
#include <time.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define NAME_LEN 32

#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE    "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN    "\x1B[36m"
#define RESET   "\x1B[0m"

// Message types
typedef enum {
    MSG_JOIN,
    MSG_LEAVE,
    PUBLIC_MESSAGE,
    PRIVATE_MESSAGE,
    GROUP_MESSAGE,
    STATUS_MESSAGE
} MessageType;

// Message structure
typedef struct {
    MessageType type;
    char sender[NAME_LEN];
    char receiver[NAME_LEN];
    char content[BUFFER_SIZE];
} Message;

// Client structure
typedef struct {
    int socket;
    char name[NAME_LEN];
    char color[10];
} Client;

#endif