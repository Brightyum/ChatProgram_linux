#include "client_vars.h"

// 여기서 실제 메모리 할당이 일어남 (초기화)
GtkWidget *window = NULL;
GtkWidget *txt_view = NULL;
GtkWidget *entry_msg = NULL;
GtkWidget *entry_name = NULL;
GtkWidget *btn_connect = NULL;
GtkTextBuffer *buffer = NULL;

int sock = -1;
int is_running = 0;
FILE *recv_fp = NULL;