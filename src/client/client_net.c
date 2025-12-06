#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../../include/common/protocol.h"
#include "client_vars.h"
#include "client_ui.h" // update_chat_ui 호출을 위해 필요

void *send_file_thread_func(void *arg) {
    char *filepath = (char *)arg;
    char *filename = strrchr(filepath, '/');
    if (filename) filename++;
    else filename = filepath;

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        perror("File open error");
        return NULL;
    }

    Packet p;
    p.type = REQ_FILE_START;
    p.room_id = 0;
    
    // 주의: GTK 위젯 접근은 메인 스레드 권장이지만, 읽기만 하므로 여기선 단순화 유지
    // 안전하게 하려면 gdk_threads_enter/leave 혹은 복사본 사용 필요
    const char* nickname = gtk_entry_get_text(GTK_ENTRY(entry_name)); 
    strcpy(p.name, nickname);
    
    strcpy(p.data, filename);
    p.data_len = strlen(filename);
    write(sock, &p, sizeof(Packet));
    usleep(10000); 

    while (!feof(fp)) {
        int read_len = fread(p.data, 1, BUF_SIZE, fp);
        if (read_len > 0) {
            p.type = REQ_FILE_DATA;
            p.data_len = read_len;
            write(sock, &p, sizeof(Packet));
            usleep(1000);
        }
    }

    p.type = REQ_FILE_END;
    p.data_len = 0;
    write(sock, &p, sizeof(Packet));

    fclose(fp);
    free(filepath);

    ChatData *info = (ChatData*)malloc(sizeof(ChatData));
    strcpy(info->name, "SYSTEM");
    sprintf(info->msg, "File sent: %s", filename);
    g_idle_add(update_chat_ui, info);

    return NULL;
}

void *recv_thread(void *arg) {
    Packet packet;
    while (is_running) {
        int str_len = read(sock, &packet, sizeof(Packet));
        if (str_len <= 0) {
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
                ChatData *chat = (ChatData*)malloc(sizeof(ChatData));
                strcpy(chat->name, packet.name);
                strcpy(chat->msg, packet.data);
                g_idle_add(update_chat_ui, chat);
                break;
            }
           case REQ_FILE_START: {
                // [수정 1] 버퍼 크기를 넉넉하게 늘림 (BUF_SIZE + 여유분)
                char save_name[BUF_SIZE + 50]; 
                
                // [수정 2] sprintf 대신 snprintf 사용 (길이 제한을 걸어 안전하게 복사)
                snprintf(save_name, sizeof(save_name), "down_%s", packet.data);

                recv_fp = fopen(save_name, "wb");

                ChatData *info = (ChatData*)malloc(sizeof(ChatData));
                strcpy(info->name, "SYSTEM");
                
                // [수정 3] 메시지도 snprintf로 안전하게 처리 (너무 길면 알아서 자름)
                snprintf(info->msg, sizeof(info->msg), "Receiving file: %.900s..", packet.data);
                
                g_idle_add(update_chat_ui, info);
                break;
            }
            case REQ_FILE_DATA: {
                if (recv_fp != NULL) fwrite(packet.data, 1, packet.data_len, recv_fp);
                break;
            }
            case REQ_FILE_END: {
                if (recv_fp != NULL) {
                    fclose(recv_fp);
                    recv_fp = NULL;
                    ChatData *info = (ChatData*)malloc(sizeof(ChatData));
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