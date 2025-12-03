// src/server/server_main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "../../include/common/protocol.h" // 경로에 주의하세요

// 연결된 클라이언트의 상태를 관리하기 위한 구조체
typedef struct {
    int sock;               // 소켓 디스크립터 (-1이면 비어있음)
    int room_id;            // 현재 위치한 방 (0: 로비)
    char name[NAME_SIZE];   // 사용자 닉네임
} ClientState;

ClientState clients[MAX_CLIENTS]; // 클라이언트 목록 관리

// 클라이언트 목록 초기화
void init_clients() {
    for(int i=0; i<MAX_CLIENTS; i++) {
        clients[i].sock = -1;
        clients[i].room_id = 0; // 기본은 로비
        memset(clients[i].name, 0, NAME_SIZE);
    }
}

// 빈 슬롯 찾기 및 등록
int add_client(int sock) {
    for(int i=0; i<MAX_CLIENTS; i++) {
        if (clients[i].sock == -1) {
            clients[i].sock = sock;
            clients[i].room_id = 0; // 로비 입장
            sprintf(clients[i].name, "Guest%d", sock); // 임시 이름
            return i;
        }
    }
    return -1; // 꽉 참
}

// 소켓 번호로 인덱스 찾기
int get_client_index(int sock) {
    for(int i=0; i<MAX_CLIENTS; i++) {
        if (clients[i].sock == sock) return i;
    }
    return -1;
}

// 클라이언트 제거
void remove_client(int index) {
    if (index < 0 || index >= MAX_CLIENTS) return;
    close(clients[index].sock);
    clients[index].sock = -1;
    clients[index].room_id = 0;
    memset(clients[index].name, 0, NAME_SIZE);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    
    fd_set reads, copy_reads;
    int fd_max, fd_num, i, j, str_len;
    
    Packet recv_packet; // 수신용 패킷 버퍼
    Packet send_packet; // 송신용 패킷 버퍼

    init_clients();

    // 1. 소켓 생성 및 설정
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
                
                // Case A: 새로운 연결
                if (i == server_sock) {
                    addr_size = sizeof(client_addr);
                    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
                    
                    if (add_client(client_sock) == -1) {
                        printf("Max clients reached. Connection rejected.\n");
                        close(client_sock);
                    } else {
                        FD_SET(client_sock, &reads);
                        if (fd_max < client_sock) fd_max = client_sock;
                        printf("Connected client: %d\n", client_sock);
                    }
                }
                // Case B: 데이터 수신
                else {
                    str_len = read(i, &recv_packet, sizeof(Packet)); // 구조체 단위 수신
                    int idx = get_client_index(i);

                    // 연결 종료
                    if (str_len <= 0) {
                        FD_CLR(i, &reads);
                        remove_client(idx);
                        printf("Closed client: %d\n", i);
                    } 
                    else {
                        // 요청 타입에 따른 처리
                        switch(recv_packet.type) {
                            case REQ_LOGIN:
                                strcpy(clients[idx].name, recv_packet.name);
                                printf("[LOGIN] Socket %d is now '%s'\n", i, clients[idx].name);
                                break;

                            case REQ_JOIN_ROOM:
                                clients[idx].room_id = recv_packet.room_id;
                                printf("[JOIN] %s moved to Room %d\n", clients[idx].name, recv_packet.room_id);
                                break;

                            case REQ_CHAT:
                                printf("[MSG] Room %d - %s: %s\n", clients[idx].room_id, clients[idx].name, recv_packet.data);
                                
                                // 브로드캐스팅 (같은 방에 있는 사람에게만)
                                send_packet = recv_packet; // 내용 복사
                                // 서버가 보낼 때는 보낸 사람 이름을 확실히 박아줌
                                strcpy(send_packet.name, clients[idx].name); 

                                for (j = 0; j < MAX_CLIENTS; j++) {
                                    // 존재하는 클라이언트 && 나 자신 제외 && 같은 방
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