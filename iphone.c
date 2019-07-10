/*Ubuntuでのコンパイル方法
  sudo apt-get install libgtk-3-dev     //(GTKをインストール)
  gcc -o iphone iphone.c $(pkg-config --cflags --libs gtk+-3.0)
  ./iphone
*/
#pragma execution_character_set("utf-8")

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h> 
#include <signal.h>
#include <unistd.h>
#include <complex.h>
#include <sox.h>
#include <string.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <err.h>
#include <time.h>

#define X 512

//エラー処理用関数
void die(char* s){
  perror(s);
  exit(1);
}

GtkWidget *notebook;//親ウィジェット notebookを指定
GtkWidget *window;//親ウィジェット windowを指定
GtkWidget *entry_server;
GtkWidget *accept_button;
GtkWidget *no_accept_button;
GtkWidget *closebutton;//引数として取れないのでグローバルで宣言
FILE* rec_p;
FILE* play_p;
char cmd1[100];
gint pos = 2;//
gboolean show_t = TRUE;//TRUE or FALSE
gboolean show_b = TRUE;//TRUE or FALSE
int a=0;

typedef struct _Entrybuf {
  char buf1[256];//IPアドレス
  int buf2;//ポート番号
} Entrybuf;

//pitchを上げる関数
void voice_pitch_up(GtkRadioButton *radiobutton){
  char x[100] = "play -t raw -b 16 -c 1 -e s -r 44100 - pitch 1400";
  strcpy(cmd1,x);
}

//pitchをもとに戻す関数
void voice_pitch_normal(GtkRadioButton *radiobutton){
  char x[100] = "play -t raw -b 16 -c 1 -e s -r 44100 - ";
  strcpy(cmd1,x);
}

//pitchを下げる関数
void voice_pitch_down(GtkRadioButton *radiobutton){
  char x[100] = "play -t raw -b 16 -c 1 -e s -r 44100 - pitch -600";
  strcpy(cmd1,x);
}

//待っている関数
static void cb_buttons_clicked(GtkWidget *buttons){
  char text[10]="Waiting...";
  gtk_entry_set_text(GTK_ENTRY(entry_server),text);
}

static void cb_accept_clicked(GtkWidget *accept_button){
  a=1;
}

static void cb_no_accept_clicked(GtkWidget *no_accept_button){
  a=2;
}

static void cb_close_clicked_2(GtkWidget *closebutton, gpointer user_data){ 
  a=3;
}

//プログラムを終了する
static void cb_close_clicked_1(GtkWidget *closebutton, gpointer user_data){
  gtk_main_quit();
}

//ポート番号が10000~65535じゃないときにダイアログを表示する関数
static void show_dialog(void){
  GtkWidget* dialog;
  dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Port Number must be 10000~65535");
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

//Clientの関数
void *cb_client_call_clicked(void* _eb)
{
  Entrybuf eb = *(Entrybuf*)_eb;

  int s = socket(PF_INET, SOCK_STREAM, 0);
  if (s == -1) die("socket");
  
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  if (inet_aton(eb.buf1, &addr.sin_addr) == 0) exit(1);
  addr.sin_port = htons(eb.buf2);
  int ret = connect(s, (struct sockaddr*)&addr, sizeof(addr));
  if (ret == -1) die("socket");

  pid_t pid;
  FILE *pout1, *pout2;
  pid = fork();
  if(pid == -1){
      die("fork");
  }else if(pid == 0){
      char buf[1];
      pout1 = fopen("dq1.wav", "r");
      pout2 = popen("play -t raw -b 16 -c 1 -e s -r 88200 -", "w");
      while(1){
          int fread_res = fread(buf, sizeof(char), 1, pout1);
          if(fread_res == 0){
              pout1 = fopen("dq1.wav", "r");
              continue;
          }
          fprintf(pout2, "%c", buf[0]);
      }
  }else{
      char c;
      while(a==0){
        g_signal_connect(GTK_BUTTON(closebutton), "clicked", G_CALLBACK(cb_close_clicked_2), NULL);
        if(read(s, &c, 1)==1)break;
      }
      if(a == 3){
        kill(pid, SIGINT);
        wait(NULL);
        exit(1);
      }
      if(c != 'y'){
          kill(pid, SIGINT);
          wait(NULL);
          exit(1);
      }
      else{
          kill(pid, SIGINT);
          wait(NULL);
      }
  }
  
  FILE* rec_p;
  if((rec_p = popen("rec -t raw -b 16 -c 1 -e s -r 44100 - ", "r")) == NULL) exit(1);

  FILE* play_p;
  play_p = popen(cmd1,"w");
  
  char buf[X], buf2[X];
  int i = 0;

  while(i < 1){
      int j;
      for(j=0; j<X; j++){
          fread(buf2+j, sizeof(char), 1, rec_p);
      }
      int write_res = write(s, buf2, X);
      if(write_res == -1) die("write");
      i++;
  }
  while(1){
      int j;
      int read_res = read(s, buf, X);
      if(read_res == -1) die("read");
      if(read_res == 0) break;
      for(j=0; j<X; j++){
          int write_res = fprintf(play_p, "%c", buf[j]);
          if(write_res == -1) die("write");
      }

      for(j=0; j<X; j++){
          fread(buf2+j, sizeof(char), 1, rec_p);
      }

      int write_res = write(s, buf2, X);
      if(write_res == -1) die("write");
  }

  close(s);
  pclose(rec_p);
  pclose(play_p);
}

//clientのCALLボタンが押されると起動する関数
static void cb_client_thread_call(GtkWidget *button, gpointer *entrydata)
{
  pthread_t thread_id;
  int status;
  char buf1[256];
  int buf2;
  const gchar *text1;
  const gchar *text2;
  text1 = gtk_entry_get_text(GTK_ENTRY((char*)entrydata[0]));
  text2 = gtk_entry_get_text(GTK_ENTRY((char*)entrydata[1]));
  buf2 = (int)strtol(text2, NULL, 10);
  Entrybuf eb;
  sprintf(eb.buf1, "%s", text1);
  eb.buf2 = buf2;
  if (eb.buf2 < 10000 || eb.buf2 > 65535) {
    show_dialog();
    return;
  }
  status = pthread_create(&thread_id, NULL, &cb_client_call_clicked, &eb);
   if (status == -1) {
    perror("pthread_create");
  }
}

//Serverの関数
void *cb_server_call_clicked(void* _buf)
{
  int buf1 = *(int*)_buf;

  int ss = socket(PF_INET, SOCK_STREAM, 0);
  if (ss == -1) die("socket");
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(buf1);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(ss, (struct sockaddr*)&addr, sizeof(addr));
  listen(ss,10);
  struct sockaddr_in client_addr;
  socklen_t len = sizeof(struct sockaddr_in);
  int s = accept(ss, (struct sockaddr*)&client_addr, &len);
  close(ss);

  pid_t pid;
  FILE *pout1, *pout2;
  pid = fork();
  if(pid == -1){
      die("fork");
  }
  else if(pid == 0){
    char buf[1];
    pout1 = fopen("dq1.wav", "r");
    pout2 = popen("play -t raw -b 16 -c 1 -e s -r 88200 -", "w");
    while(1){
      int fread_res = fread(buf, sizeof(char), 1, pout1);
      if(fread_res == 0){
          pout1 = fopen("dq1.wav", "r");
          continue;
      }
      fprintf(pout2, "%c", buf[0]);
    }
  }
  else
  {
    char init_text[25] = "Please push under button";
    const gchar *text1;
    gtk_entry_set_text(GTK_ENTRY(entry_server),init_text);
    while(a == 0){
        g_signal_connect(accept_button, "clicked", G_CALLBACK(cb_accept_clicked), NULL);//a=1
        g_signal_connect(no_accept_button, "clicked", G_CALLBACK(cb_no_accept_clicked), NULL);//a=2
        g_signal_connect(GTK_BUTTON(closebutton), "clicked", G_CALLBACK(cb_close_clicked_2), NULL);//a=3
    }
    if(a == 1){
      char c = 'y';
      write(s, &c, 1);
      kill(pid, SIGINT);
      wait(NULL);
    }
    else{
      char c = 'n';
      write(s, &c, 1);
      kill(pid, SIGINT);
      wait(NULL);
      exit(1);
    }
  }

  FILE* rec_p;
  if((rec_p = popen("rec -t raw -b 16 -c 1 -e s -r 44100 - ", "r")) == NULL) exit(1);

  FILE* play_p;
  play_p = popen(cmd1,"w");
  
  char buf[X], buf2[X];
  int i = 0;

  while(i < 1){
      int j;
      for(j=0; j<X; j++){
          fread(buf2+j, sizeof(char), 1, rec_p);
      }
      int write_res = write(s, buf2, X);
      if(write_res == -1) die("write");
      i++;
  }
  while(1){
      int j;
      int read_res = read(s, buf, X);
      if(read_res == -1) die("read");
      if(read_res == 0) break;
      for(j=0; j<X; j++){
          int write_res = fprintf(play_p, "%c", buf[j]);
          if(write_res == -1) die("write");
      }

      for(j=0; j<X; j++){
          fread(buf2+j, sizeof(char), 1, rec_p);
      }

      int write_res = write(s, buf2, X);
      if(write_res == -1) die("write");
  }

  close(s);
  pclose(rec_p);
  pclose(play_p);
}

//ServerのBuildボタンが押されると動作する関数
static void cb_server_thread_call(GtkWidget *button)
{
  pthread_t thread_id;//pthread
  int status, buf;
  const gchar* text;
  text = gtk_entry_get_text(GTK_ENTRY((char*)entry_server));
  buf = (int)strtol(text, NULL, 10);//ポート番号

  if (buf < 10000 || buf > 65535) {
    show_dialog();
    return;
  }

  for(int i = 0; i < 3; i++) {
    status = pthread_create(&thread_id, NULL, &cb_server_call_clicked, &buf);
    if (status == -1) {
      perror("pthread_create");
    }
  }
}

//ほぼGUIの処理のみ　IPアドレスの値だけ何もやってないから直しておく
int main(int argc, char** argv){
  GtkWidget *entry_client[2];
  GtkWidget *label;
  GtkWidget *timelabel;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *buttons;
  GtkWidget *buttonc;
  GtkWidget *radiobutton[2];
  GtkWidget *grid;//諸々のGUI部品

  GSList *group = NULL;
  char bufferl[32];
  
  gtk_init(&argc, &argv);//gtk+の初期化

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);//ウィンドウの作成
  gtk_window_set_title(GTK_WINDOW(window), "Phone");//タイトル 第１引数はGtkWindow*型でないといけないのでGTK_WINDOW関数でキャスト
  //今回はウィジェットを多く使うので、指定サイズより大きくなる
  gtk_window_set_default_size(GTK_WINDOW(window), 300, 250);//サイズ　gtk_widget_set_size_requestと違って初期値しか設定しない=userはresizeできる

  grid = gtk_grid_new();//gridを作成(電卓的な)
  gtk_container_add(GTK_CONTAINER(window), grid);//gridをウィンドウに追加

  notebook = gtk_notebook_new();//notebook(pagesが集まったウィジェット)を作成
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);//notebookのページを切り替えるタブを一番上に置く
  gtk_grid_attach(GTK_GRID(grid), notebook, 0, 0, 1, 2);//左上に横2マス,縦1マスのサイズのボタンを配置
  gtk_container_set_border_width(GTK_CONTAINER(notebook), 10);//notebookのサイズを横10マスに指定
  
  int i;
  //Pageの作成・追加
  for(i=0; i < 3; i++){
    if (i == 0) {
      sprintf(bufferl, "Client");//bufferl(char配列)に"Client"を書き込む
      vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);//垂直に5pxのboxを作成
      hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);//水平に0pxのboxを作成
      gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);//vboxの中にhboxをパックする

      label = gtk_label_new("IP Address:");//新規ラベル作成
      gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);//hboxの中にlabelをパックする
      entry_client[0] = gtk_entry_new();//エントリーウィジェット(入力と表示ができるテキストボックス)を作る
      gtk_box_pack_start(GTK_BOX(hbox), entry_client[0], TRUE, TRUE, 0);//hboxの中にエントリーウィジェットをパックする

      hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);//hboxをもう一個作る
      gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);//vboxの中にhboxをパックする
      
      label = gtk_label_new("Port Number:");//新規ラベル作成
      gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);//hboxの中にlabelをパックする
      entry_client[1] = gtk_entry_new();//エントリーウィジェット(入力と表示ができるテキストボックス)を作る
      gtk_box_pack_start(GTK_BOX(hbox), entry_client[1], TRUE, TRUE, 0);//hboxの中にエントリーウィジェットをパックする

      buttonc = gtk_button_new_with_label("CALL");//"CALL"の文字が入ったボタンを作る
      gtk_box_pack_start(GTK_BOX(vbox), buttonc, TRUE, TRUE, 0);//hboxの中にボタンをパックする
      g_signal_connect(buttonc, "clicked", G_CALLBACK(cb_client_thread_call), entry_client);//buttoncがクリックされたらcb_client_thread_callを呼ぶ
      
      label = gtk_label_new(bufferl);//新規ラベル作成
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);//notebookの後ろにvboxをbufferlの名前でページを作る
    }
    else if(i==1){
      sprintf(bufferl, "Server");//bufferlに"Server"を書き込む
      vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);//垂直方向に5pxのboxを作成
      hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);//水平方向に0pxのboxを作成
      gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);//vboxの中にhboxをパックする

      label = gtk_label_new("Port Number:");//新規ラベル作成
      gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 5);//hboxの中にlabelをパックする
      entry_server = gtk_entry_new();//エントリーウィジェット作成
      gtk_box_pack_start(GTK_BOX(hbox), entry_server, TRUE, TRUE, 0);//hboxの中にentry_serverをパックする

      buttons = gtk_button_new_with_label("Build");//新規ラベル作成
      gtk_box_pack_start(GTK_BOX(vbox), buttons, TRUE, TRUE, 0);//hboxの中にbuttonsをパックする
      g_signal_connect(buttons, "clicked", G_CALLBACK(cb_server_thread_call), NULL);//buttonsがクリックされたらcb_server_thread_callを呼ぶ
      g_signal_connect(buttons, "clicked", G_CALLBACK(cb_buttons_clicked), NULL);//buttonsがクリックされたらcb_buttons_clickedを呼ぶ

      accept_button = gtk_button_new_with_label("Accept");//新規ボタン
      gtk_box_pack_start(GTK_BOX(vbox), accept_button, TRUE, TRUE, 0);//hboxの中にaccept_buttonをパックする

      no_accept_button = gtk_button_new_with_label("No Accept");//新規ボタン
      gtk_box_pack_start(GTK_BOX(vbox), no_accept_button, TRUE, TRUE, 0);//hboxの中にno_accept_buttonをパックする

      label = gtk_label_new(bufferl);//新規ラベル作成
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);//notebookにlabelという名前の新規ページを付け足す
    }
    else if(i==2){
      char x[7] = "Option";
      vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);//垂直方向に5pxのboxを作成
      hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);//水平方向に0pxのboxを作成
      gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);//vboxの中にhboxをパックする

      radiobutton[0] = gtk_radio_button_new_with_label(group,"Pitch_up");
      gtk_box_pack_start(GTK_BOX(hbox), radiobutton[0], TRUE, TRUE, 0);
      g_signal_connect(G_OBJECT(radiobutton[0]), "toggled", G_CALLBACK(voice_pitch_up), NULL);

      radiobutton[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radiobutton[0]),"Pitch_normal");
      gtk_box_pack_start(GTK_BOX(hbox), radiobutton[1], TRUE, TRUE, 0);
      g_signal_connect(G_OBJECT(radiobutton[1]), "toggled", G_CALLBACK(voice_pitch_normal), NULL);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton[1]), TRUE);

      radiobutton[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radiobutton[0]),"Pitch_down");
      gtk_box_pack_start(GTK_BOX(hbox), radiobutton[2], TRUE, TRUE, 0);
      g_signal_connect(G_OBJECT(radiobutton[2]), "toggled", G_CALLBACK(voice_pitch_down), NULL);
      
      label = gtk_label_new(x);//新規ラベル作成
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);//notebookの後ろにvboxをbufferlの名前でページを作る
    }
  }
  
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);//notebookの初期ページを設定

  closebutton = gtk_button_new_with_label ("close");//"close"という名前のbuttonを作成
  g_signal_connect(GTK_BUTTON(closebutton), "clicked", G_CALLBACK(cb_close_clicked_1), NULL);//closebuttonが押されたときにprogramを終了
  gtk_grid_attach(GTK_GRID(grid), closebutton, 0,5,1,1);//gridにボタンを配置

  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);//ウィンドウが閉じる操作が行われたらgtk_main_quitを呼び出す
  gtk_widget_show_all(window);//ウィンドウを表示

  gtk_main();//メインループ　gtk_main_quitが呼ばれたら抜ける

  return 0;
}
