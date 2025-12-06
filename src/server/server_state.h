#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include "../../include/common/protocol.h"

typedef struct {
    int sock;
    int room_id;
    char name[NAME_SIZE];
} ClientState;

extern ClientState clients[MAX_CLIENTS];

void init_clients();
int add_client(int sock);
int get_client_index(int sock);
void remove_client(int index);

#endif