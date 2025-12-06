# 컴파일러 설정
CC = gcc

# 공통 옵션: 헤더 경로(-I) 및 스레드 사용(-pthread)
CFLAGS = -I./include -pthread

# GTK+ 3.0 설정 (클라이언트용)
GTK_FLAGS = `pkg-config --cflags --libs gtk+-3.0`

# 소스 파일 경로 정의
SRV_SRCS = src/server/server_main.c src/server/server_state.c
CLT_SRCS = src/client/client_main.c src/client/client_vars.c src/client/client_net.c src/client/client_ui.c

# 생성될 실행 파일 이름
SERVER_BIN = server
CLIENT_BIN = client

# 1. 'make'만 쳤을 때 수행할 기본 동작 (둘 다 만들기)
all: $(SERVER_BIN) $(CLIENT_BIN)

# 2. 서버 컴파일 규칙
$(SERVER_BIN): $(SRV_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# 3. 클라이언트 컴파일 규칙 (GTK 플래그 포함)
$(CLIENT_BIN): $(CLT_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(GTK_FLAGS)

# 4. 'make clean' 실행 시 청소 규칙
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)