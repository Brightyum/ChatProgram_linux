#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "server_state.h"

ClientState clients[MAX_CLIENTS];

void init_clients() {
    for(int i=0; i<MAX_CLIENTS; i++) {
        clients[i].sock = -1;
        clients[i].room_id = 0;
        memset(clients[i].name, 0, NAME_SIZE);
    }
}

int add_client(int sock) {
    for(int i=0; i<MAX_CLIENTS; i++) {
        if (clients[i].sock == -1) {
            clients[i].sock = sock;
            clients[i].room_id = 0;
            sprintf(clients[i].name, "Guest%d", sock);
            return i;
        }
    }
    return -1;
}

int get_client_index(int sock) {
    for(int i=0; i<MAX_CLIENTS; i++) {
        if (clients[i].sock == sock) return i;
    }
    return -1;
}

void remove_client(int index) {
    if (index < 0 || index >= MAX_CLIENTS) return;
    close(clients[index].sock);
    clients[index].sock = -1;
    clients[index].room_id = 0;
    memset(clients[index].name, 0, NAME_SIZE);
}