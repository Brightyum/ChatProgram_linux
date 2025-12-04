#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../../include/common/protocol.h"

FILE *recv_fp = NULL; // 파일을 받을 때 쓸 파일 포인터

// --- 전역 변수 (GUI 위젯 및 소켓) ---
GtkWidget *window;
GtkWidget *txt_view;      // 채팅 내용 표시
GtkWidget *entry_msg;     // 메시지 입력창
GtkWidget *entry_name;    // 이름 입력창
GtkWidget *btn_connect;   // 접속 버튼
GtkTextBuffer *buffer;    // txt_view의 버퍼

int sock = -1;            // 소켓 디스크립터
int is_running = 0;       // 스레드 제어 플래그

// --- GUI 업데이트를 위한 구조체 ---
typedef struct {
    char name[NAME_SIZE];
    char msg[BUF_SIZE];
} ChatData;

// 파일을 쪼개서 보내는 스레드 함수
void *send_file_thread_func(void *arg) {
    char *filepath = (char *)arg;
    char *filename = strrchr(filepath, '/'); // 경로에서 파일명만 추출
    if (filename) filename++;
    else filename = filepath;

    FILE *fp = fopen(filepath, "rb"); // 바이너리 읽기 모드
    if (!fp) {
        perror("File open error");
        return NULL;
    }

    // 1. START 패킷 전송 (파일명)
    Packet p;
    p.type = REQ_FILE_START;
    p.room_id = 0;
    strcpy(p.name, gtk_entry_get_text(GTK_ENTRY(entry_name)));
    strcpy(p.data, filename); // 데이터 영역에 파일명을 담음
    p.data_len = strlen(filename);
    write(sock, &p, sizeof(Packet));
    
    // 약간의 딜레이 (패킷 순서 꼬임 방지)
    usleep(10000); 

    // 2. DATA 패킷 전송 (파일 내용)
    while (!feof(fp)) {
        int read_len = fread(p.data, 1, BUF_SIZE, fp);
        if (read_len > 0) {
            p.type = REQ_FILE_DATA;
            p.data_len = read_len; // 읽은 만큼만 길이 설정
            write(sock, &p, sizeof(Packet));
            usleep(1000); // 전송 속도 조절 (플러딩 방지)
        }
    }

    // 3. END 패킷 전송
    p.type = REQ_FILE_END;
    p.data_len = 0;
    write(sock, &p, sizeof(Packet));

    fclose(fp);
    free(filepath); // 동적 할당된 경로 메모리 해제

    // GUI에 "전송 완료" 알림 (g_idle_add 사용)
    ChatData *info = (ChatData*)malloc(sizeof(ChatData));
    strcpy(info->name, "SYSTEM");
    sprintf(info->msg, "File sent: %s", filename);
    g_idle_add(update_chat_ui, info);

    return NULL;
}

// "파일 전송" 버튼 클릭 시 호출
void on_file_btn_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Select File",
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "Cancel", GTK_RESPONSE_CANCEL,
                                         "Open", GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // 스레드 생성하여 전송 (GUI 프리징 방지)
        pthread_t t_id;
        // filepath는 스레드 안에서 free 해야 함
        char *path_copy = strdup(filepath); 
        pthread_create(&t_id, NULL, send_file_thread_func, path_copy);
        pthread_detach(t_id);
        
        g_free(filepath);
    }
    gtk_widget_destroy(dialog);
}

// --------------------------------------------------------
// 1. GUI 업데이트 함수 (메인 스레드에서 실행됨)
// --------------------------------------------------------
gboolean update_chat_ui(gpointer data) {
    ChatData *chat = (ChatData *)data;
    GtkTextIter end;
    char full_msg[BUF_SIZE + NAME_SIZE + 10];

    // 버퍼의 끝 위치를 가져옴
    gtk_text_buffer_get_end_iter(buffer, &end);

    // 텍스트 포맷팅: [Name] : Message
    sprintf(full_msg, "[%s] : %s\n", chat->name, chat->msg);
    
    // 텍스트 추가
    gtk_text_buffer_insert(buffer, &end, full_msg, -1);

    // 스크롤을 항상 아래로
    GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(txt_view), mark, 0.0, TRUE, 0.0, 1.0);
    gtk_text_buffer_delete_mark(buffer, mark);

    free(chat); // 힙 메모리 해제
    return FALSE; // 한 번만 실행하고 제거
}

// --------------------------------------------------------
// 2. 네트워크 수신 스레드 (백그라운드)
// --------------------------------------------------------
void *recv_thread(void *arg) {
    Packet packet;
    while (is_running) {
        int str_len = read(sock, &packet, sizeof(Packet));
        if (str_len <= 0) {
            // 연결 끊김 처리
            ChatData *info = (ChatData*)malloc(sizeof(ChatData));
            strcpy(info->name, "SYSTEM");
            strcpy(info->msg, "Disconnected from server.");
            g_idle_add(update_chat_ui, info);
            
            close(sock);
            is_running = 0;
            break;
        }

        switch (packet.type) {
            case REQ_CHAT: {
                ChatData *chat = (ChatData*)malloc(sizeof(chatData));
                strcpy(chat->name, packet.name);
                strcpy(chat->msg, packet.data);
                g_idle_add(update_chat_ui, chat);
                break;
            }

            case REQ_FILE_START: {
                char save_name[BUF_SIZE];
                sprintf(save_name, "down_%s", packet.data);

                recv_fp = fopen(save_name, "wb");

                ChatData *info = (ChatData*)malloc(sizeof(ChatData));
                strcpy(info->name, "SYSTEM");
                sprintf(info->msg, "Receiving file: %s..", packet.data);
                g_idle_add(update_chat_ui, info);
                break;
            }

            case REQ_FILE_DATA: {
                if (recv_fp != NULL) {
                    fwrite(packet.data, 1, packet.data_len, recv_fp);
                }
                break;
            }

            case REQ_FILE_END: {
                if (recv_fp != NULL) {
                    fclose(recv_fp);
                    recv_fp = NULL;

                    ChatData *info = (ChatData)malloc(sizeof(ChatData));
                    strcpy(info->name, "SYSTEM");
                    strcpy(info->msg, "File download complete");
                    g_idle_add(update_chat_ui, info);
                }
                break;
            }
        }
    }
    return NULL;
}

// --------------------------------------------------------
// 3. 이벤트 핸들러 (버튼 클릭 등)
// --------------------------------------------------------

// 접속 버튼 클릭 시
void on_connect_clicked(GtkWidget *widget, gpointer data) {
    const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
    if (strlen(name) == 0) return;

    struct sockaddr_in serv_addr;
    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 로컬호스트
    serv_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        // 접속 실패 메시지 출력
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, "Connection Failed!\n", -1);
        return;
    }

    // 로그인 패킷 전송
    Packet p;
    p.type = REQ_LOGIN;
    p.room_id = 0;
    strcpy(p.name, name);
    memset(p.data, 0, BUF_SIZE);
    write(sock, &p, sizeof(Packet));

    // 수신 스레드 시작
    is_running = 1;
    pthread_t t_id;
    pthread_create(&t_id, NULL, recv_thread, NULL);
    pthread_detach(t_id);

    // UI 상태 변경
    gtk_widget_set_sensitive(btn_connect, FALSE);
    gtk_widget_set_sensitive(entry_name, FALSE);
    
    // 성공 메시지
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, "Connected to Server!\n", -1);
}

// 전송 버튼(또는 엔터키) 클릭 시
void on_send_clicked(GtkWidget *widget, gpointer data) {
    if (sock == -1) return;

    const char *msg = gtk_entry_get_text(GTK_ENTRY(entry_msg));
    const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));

    if (strlen(msg) == 0) return;

    Packet p;
    p.type = REQ_CHAT;
    p.room_id = 0;
    strcpy(p.name, name);
    strcpy(p.data, msg);

    write(sock, &p, sizeof(Packet));

    // 입력창 비우기
    gtk_entry_set_text(GTK_ENTRY(entry_msg), "");
}

// --------------------------------------------------------
// 4. 메인 함수 (UI 레이아웃 구성)
// --------------------------------------------------------
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // 윈도우 생성
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Team Project - GTK Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 레이아웃 (Vertical Box)
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // 1. 이름 입력 & 접속 버튼 (Horizontal Box)
    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_name), "Enter Nickname");
    btn_connect = gtk_button_new_with_label("Connect");
    
    gtk_box_pack_start(GTK_BOX(hbox_top), entry_name, TRUE, TRUE, 0); // Expand
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_connect, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_top, FALSE, FALSE, 5);

    // 2. 채팅창 (Scrolled Window + Text View)
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    txt_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(txt_view), FALSE); // 읽기 전용
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(txt_view), FALSE);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_view));
    
    gtk_container_add(GTK_CONTAINER(scrolled_window), txt_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0); // 화면 채움

    // 3. 메시지 입력 & 전송 버튼 (Horizontal Box)
    GtkWidget *hbox_bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    entry_msg = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_msg), "Type message...");
    GtkWidget *btn_send = gtk_button_new_with_label("Send");

    GtkWidget *btn_file = gtk_button_new_with_label("Send File"); 
    GtkWidget *btn_send = gtk_button_new_with_label("Send");

    gtk_box_pack_start(GTK_BOX(hbox_bottom), entry_msg, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_bottom), btn_file, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_bottom), btn_send, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 5);

    // 시그널 연결
    g_signal_connect(btn_connect, "clicked", G_CALLBACK(on_connect_clicked), NULL);
    g_signal_connect(btn_file, "clicked", G_CALLBACK(on_file_btn_clicked), NULL);
    g_signal_connect(btn_send, "clicked", G_CALLBACK(on_send_clicked), NULL);
    g_signal_connect(entry_msg, "activate", G_CALLBACK(on_send_clicked), NULL); // 엔터키 처리

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}