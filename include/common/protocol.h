// include/common/protocol.h
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define PORT 3490
#define BUF_SIZE 1024
#define NAME_SIZE 20
#define MAX_CLIENTS 10

// 요청 타입 정의
typedef enum {
    REQ_LOGIN,      // 0: 입장 (닉네임 설정)
    REQ_JOIN_ROOM,  // 1: 방 이동
    REQ_CHAT,       // 2: 채팅 메시지
    REQ_EXIT        // 3: 퇴장
} RequestType;

// 패킷 구조체 (서버와 클라이언트가 이 구조대로 데이터를 주고받음)
typedef struct {
    int type;               // RequestType
    int room_id;            // 0: 로비, 1~N: 특정 방
    char name[NAME_SIZE];   // 보낸 사람 이름
    char data[BUF_SIZE];    // 메시지 내용
} Packet;

#endif