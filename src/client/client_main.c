#include <gtk/gtk.h>
#include "client_vars.h"
#include "client_ui.h"
#include <locale.h>

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Team Project - GTK Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_name), "Enter Nickname");
    btn_connect = gtk_button_new_with_label("Connect");
    gtk_box_pack_start(GTK_BOX(hbox_top), entry_name, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_connect, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_top, FALSE, FALSE, 5);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    txt_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(txt_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(txt_view), FALSE);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_view));
    gtk_container_add(GTK_CONTAINER(scrolled_window), txt_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    GtkWidget *hbox_bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    entry_msg = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_msg), "Type message...");
    GtkWidget *btn_send = gtk_button_new_with_label("Send");
    GtkWidget *btn_file = gtk_button_new_with_label("Send File"); 

    gtk_box_pack_start(GTK_BOX(hbox_bottom), entry_msg, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_bottom), btn_file, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_bottom), btn_send, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 5);

    g_signal_connect(btn_connect, "clicked", G_CALLBACK(on_connect_clicked), NULL);
    g_signal_connect(btn_file, "clicked", G_CALLBACK(on_file_btn_clicked), NULL);
    g_signal_connect(btn_send, "clicked", G_CALLBACK(on_send_clicked), NULL);
    g_signal_connect(entry_msg, "activate", G_CALLBACK(on_send_clicked), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}