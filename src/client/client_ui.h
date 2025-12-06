#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include <gtk/gtk.h>

gboolean update_chat_ui(gpointer data);
void on_connect_clicked(GtkWidget *widget, gpointer data);
void on_send_clicked(GtkWidget *widget, gpointer data);
void on_file_btn_clicked(GtkWidget *widget, gpointer data);

#endif