#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/common/protocol.h"
#include "client_vars.h"
#include "client_net.h" // recv_thread 등 사용
#include "client_ui.h"

gboolean update_chat_ui(gpointer data) {
    ChatData *chat = (ChatData *)data;
    GtkTextIter end;
    char full_msg[BUF_SIZE + NAME_SIZE + 10];

    gtk_text_buffer_get_end_iter(buffer, &end);
    sprintf(full_msg, "[%s] : %s\n", chat->name, chat->msg);
    gtk_text_buffer_insert(buffer, &end, full_msg, -1);

    GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(txt_view), mark, 0.0, TRUE, 0.0, 1.0);
    gtk_text_buffer_delete_mark(buffer, mark);

    free(chat);
    return FALSE;
}

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
        pthread_t t_id;
        char *path_copy = strdup(filepath); 
        pthread_create(&t_id, NULL, send_file_thread_func, path_copy);
        pthread_detach(t_id);
        g_free(filepath);
    }
    gtk_widget_destroy(dialog);
}

void on_connect_clicked(GtkWidget *widget, gpointer data) {
    const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
    if (strlen(name) == 0) return;

    struct sockaddr_in serv_addr;
    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, "Connection Failed!\n", -1);
        return;
    }

    Packet p;
    p.type = REQ_LOGIN;
    p.room_id = 0;
    strcpy(p.name, name);
    memset(p.data, 0, BUF_SIZE);
    write(sock, &p, sizeof(Packet));

    is_running = 1;
    pthread_t t_id;
    pthread_create(&t_id, NULL, recv_thread, NULL);
    pthread_detach(t_id);

    gtk_widget_set_sensitive(btn_connect, FALSE);
    gtk_widget_set_sensitive(entry_name, FALSE);
    
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, "Connected to Server!\n", -1);
}

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
    gtk_entry_set_text(GTK_ENTRY(entry_msg), "");
}