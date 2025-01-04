#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define NAME_LEN 32

typedef enum {
    MSG_JOIN,
    MSG_LEAVE,
    MSG_PUBLIC,
    MSG_PRIVATE
} MessageType;

typedef struct {
    MessageType type;
    char sender[NAME_LEN];
    char content[BUFFER_SIZE];
} Message;

typedef struct {
    int socket;
    char name[NAME_LEN];
} Client;

#endif
