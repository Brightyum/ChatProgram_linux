#ifndef CLIENT_VARS_H
#define CLIENT_VARS_H

#include <gtk/gtk.h>
#include <stdio.h>

// extern을 붙여서 "이 변수는 다른 곳에 실체가 있다"라고 알림
extern GtkWidget *window;
extern GtkWidget *txt_view;
extern GtkWidget *entry_msg;
extern GtkWidget *entry_name;
extern GtkWidget *btn_connect;
extern GtkTextBuffer *buffer;

extern int sock;
extern int is_running;
extern FILE *recv_fp;

// UI 업데이트용 구조체도 여기서 공유
typedef struct {
    char name[20];
    char msg[1024];
} ChatData;

#endif