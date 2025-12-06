#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "../../include/common/protocol.h"
#include "server_state.h"

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    
    fd_set reads, copy_reads;
    int fd_max, fd_num, i, j, str_len;
    
    Packet recv_packet;
    Packet send_packet;

    init_clients();

    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() error"); exit(1);
    }
    if (listen(server_sock, 5) == -1) {
        perror("listen() error"); exit(1);
    }

    FD_ZERO(&reads);
    FD_SET(server_sock, &reads);
    fd_max = server_sock;

    printf("Multi-Room Chat Server Started on Port %d...\n", PORT);

    while (1) {
        copy_reads = reads;
        if ((fd_num = select(fd_max + 1, &copy_reads, 0, 0, NULL)) == -1) break;
        if (fd_num == 0) continue;

        for (i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &copy_reads)) {
                if (i == server_sock) {
                    addr_size = sizeof(client_addr);
                    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
                    
                    if (add_client(client_sock) == -1) {
                        printf("Max clients reached.\n");
                        close(client_sock);
                    } else {
                        FD_SET(client_sock, &reads);
                        if (fd_max < client_sock) fd_max = client_sock;
                        printf("Connected client: %d\n", client_sock);
                    }
                }
                else {
                    str_len = read(i, &recv_packet, sizeof(Packet));
                    int idx = get_client_index(i);

                    if (str_len <= 0) {
                        FD_CLR(i, &reads);
                        remove_client(idx);
                        printf("Closed client: %d\n", i);
                    } 
                    else {
                        switch(recv_packet.type) {
                            case REQ_LOGIN:
                                strcpy(clients[idx].name, recv_packet.name);
                                printf("[LOGIN] %s\n", clients[idx].name);
                                break;
                            case REQ_JOIN_ROOM:
                                clients[idx].room_id = recv_packet.room_id;
                                printf("[JOIN] %s -> Room %d\n", clients[idx].name, recv_packet.room_id);
                                break;
                            case REQ_CHAT:
                                send_packet = recv_packet;
                                strcpy(send_packet.name, clients[idx].name);
                                for (j = 0; j < MAX_CLIENTS; j++) {
                                    if (clients[j].sock != -1 && clients[j].sock != i && 
                                        clients[j].room_id == clients[idx].room_id) {
                                        write(clients[j].sock, &send_packet, sizeof(Packet));
                                    }
                                }
                                break;
                            case REQ_FILE_START:
                            case REQ_FILE_DATA:
                            case REQ_FILE_END:
                                send_packet = recv_packet;
                                for (j = 0; j < MAX_CLIENTS; j++) {
                                    if (clients[j].sock != -1 && clients[j].sock != i && 
                                        clients[j].room_id == clients[idx].room_id) {
                                        write(clients[j].sock, &send_packet, sizeof(Packet));
                                    }
                                }
                                break;
                        }
                    }
                }
            }
        }
    }
    close(server_sock);
    return 0;
}