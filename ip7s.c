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

//#include "bandpass.h"
#define N 512

void die(char *s){
    perror(s);
    exit(1);
}

// check if port number is between 10000 and 60000
void portnum_check(int port_num){
    if(port_num < 10000 || 60000 < port_num){
        die("Port number must be between 10000 and 60000");
    }
}

// sound processing option
void sound_option(char *cmd){
    char buf[100];
    while(1){
        printf("Select sound option [low/normal/high]\n");
        scanf("%s", buf);
        if(!strcmp(buf, "low")){
            strcat(cmd, " pitch -600");
            return ;
        }else if(!strcmp(buf, "normal")){
            return ;
        }else if(!strcmp(buf, "high")){
            strcat(cmd, " pitch 1400");
            return ;
        }else{
            printf("Invalid argument\n");
        }
    }
}

// janken function
void janken(int s){
    char my_hand[1], opp_hand[1];
    // open new terminal
    system("clear");
	printf("\n\n\nJanken has started. Please wait\n\n");
    sleep(3);

    system("clear");
    printf("\n\n\nPlease enter your hand. (Rock:r, Paper:p, Scissors: s)\n\n\n");
    // get my hand
    while(read(0, my_hand, 1)){
        if(my_hand[0] == 'r' || my_hand[0] == 'p' || my_hand[0] == 's'){
            printf("Your hand is %c\n", my_hand[0]);
            break;
        }
        system("clear");
        printf("\n\n\nPlease enter your hand again. (Rock:r, Paper:p, Scissors: s)\n\n\n");
    }

    // send my hand to opponent and recieve opponent`s hand
    //writeがブロッキングしてるっぽい　問題はない?
    int write_res = write(s, my_hand, 1);
    if(write_res == -1) die("write");
    while(1){
        int read_res = read(s, opp_hand, 1);
        if(read_res == -1) die("read");
        if(opp_hand[0] == 'r' || opp_hand[0] == 'p' || opp_hand[0] == 's') break;
    }

    // judge
    if(my_hand[0] == opp_hand[0]){
        // draw
        printf("\n\n\n\n\nDraw\n\n\n\n\n");
    }
    else if((my_hand[0] == 'r' && opp_hand[0] == 's') || (my_hand[0] == 's' && opp_hand[0] == 'p') || (my_hand[0] == 'p' && opp_hand[0] == 'r')){
        // YOU WIN !!!!!
        printf("\n\n\n\n\nWin!!!\n(You: %c Opponent: %c)\n\n\n\n", my_hand[0], opp_hand[0]);

        FILE *fp1, *fp2;
        if((fp1 = popen("tybg HONDA_WIN.MOV", "w")) == NULL ) die("popen");
        sleep(9);
        if((fp2 = popen("tybg off", "w")) == NULL ) die("popen");
    }
    else{
        // YOU LOSE !!!!!!
        printf("\n\n\n\n\nLose!!\n(You: %c Opponent: %c)\n\n\n\n", my_hand[0], opp_hand[0]);

        FILE *fp1, *fp2;
        if(my_hand[0] == 'r'){
            if((fp1 = popen("tybg HONDA_P.MOV", "w")) == NULL ) die("popen");
            sleep(17);
            if((fp2 = popen("tybg off", "w")) == NULL ) die("popen");
        }
        else if(my_hand[0] == 'p'){
            if((fp1 = popen("tybg HONDA_S.MOV", "w")) == NULL ) die("popen");
            sleep(18);
            if((fp2 = popen("tybg off", "w")) == NULL ) die("popen");
        }
        else{
            if((fp1 = popen("tybg HONDA_G.MOV", "w")) == NULL ) die("popen");
            sleep(20);
            if((fp2 = popen("tybg off", "w")) == NULL ) die("popen");
        }
    }
}


void command(char cmd, int s){
    if(cmd == 'd'){
        FILE *pout = popen("play dq1.wav", "r");
    }else if(cmd == 'q'){
        FILE *pout = popen("pkill play", "r");
    }else if(cmd == 'j'){
        janken(s);
    }else if(cmd == 'w'){
        FILE *pout = popen("play laugh.wav", "r");
    }
}

// server setting
int server_set(int port_num){
    int ss, s;

    portnum_check(port_num);

    ss = socket(PF_INET, SOCK_STREAM, 0);
    if(ss == -1) die("fail to make socket");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // this is ipv4 address
    addr.sin_port = htons(port_num); // port is argv[1]
    addr.sin_addr.s_addr = INADDR_ANY;
    int bind_res = bind(ss, (struct sockaddr *)&addr, sizeof(addr));
    if(bind_res == -1) die("failed to bind");

    int listen_res = listen(ss, 10);
    if(listen_res == -1) die("failed to listen");

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    s = accept(ss, (struct sockaddr *)&client_addr, &len);

    return s;
}

int client_set(char *ip_add, int port_num){
    int s;

    portnum_check(port_num);

    s = socket(PF_INET, SOCK_STREAM, 0);
    if(s == -1) die("fail to make socket");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // this is ipv4 address
    int inet_res = inet_aton(ip_add, &addr.sin_addr); // ip adress is ~
    if(inet_res == -1) die("fail inet");

    addr.sin_port = htons(port_num); // port is ~
    int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if(ret == -1) die("fail to connect");

    return s;
}

void communicate(int s, char *cmd){
    FILE *pin = popen(cmd, "r");
    if(pin == NULL) die("popen");
    FILE *pout = popen("play -t raw -b 16 -c 1 -e s -r 44100 - gain -h", "w");
    if(pout == NULL) die("popenw");

    char buf[N], buf2[N];
    int i = 0;

    while(i < 1){
        int j;
        for(j=0; j<N; j++){
            fread(buf2+j, sizeof(char), 1, pin);
        }
        int write_res = write(s, buf2, N);
        if(write_res == -1) die("write");
        i++;
    }
    while(1){
        int j;
        int read_res = read(s, buf, N);
        if(read_res == -1) die("read");
        if(read_res == 0) break;
        for(j=0; j<N; j++){
            int write_res = fprintf(pout, "%c", buf[j]);
            if(write_res == -1) die("write");
        }

        for(j=0; j<N; j++){
            fread(buf2+j, sizeof(char), 1, pin);
        }

        int write_res = write(s, buf2, N);
        if(write_res == -1) die("write");
    }

    close(s);
    pclose(pin);
    pclose(pout);
}

int main(int argc, char **argv){
    int s, ss;
    char cmd[100] = "rec -t raw -b 16 -c 1 -e s -r 44100 - highpass 300 lowpass 3400";

    // sound_option(cmd);

    if(argc == 2){
        s = server_set(atoi(argv[1])); // server setting

        // while calling
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
            printf("Do you accept?[y/n]\n");
            while((c = getchar())!=EOF){
                if(c == 'y'){
                    write(s, &c, 1);
                    break;
                }else if(c == 'n'){
                    write(s, &c, 1);
                    kill(pid, SIGINT);
                    wait(NULL);
                    return 0;
                }else if(c == '\n'){
                    continue;
                }
                else printf("Invalid argument\n");
            }
            kill(pid, SIGINT);
            wait(NULL);
        }

    }else if(argc == 3){
        s = client_set(argv[1], atoi(argv[2]));

        // while calling
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
            read(s, &c, 1);
            if(c != 'y'){
                kill(pid, SIGINT);
                wait(NULL);
                return 0;
            }else{
                kill(pid, SIGINT);
                wait(NULL);
            }
        }
    }else{
        die("invalid argument");
    }

    sleep(1);

    
    int s2;
    if(argc == 2){
        s2 = server_set(atoi(argv[1])+1);
    }else if(argc == 3){
        s2 = client_set(argv[1], atoi(argv[2])+1);
    }
    

    pid_t pid, pid2;
    pid = fork();
    if(pid == -1){
        die("fork");
    }else if(pid == 0){
        char c, buf;
        pid2 = fork();

        while(1){
            if(pid2 == -1){
                die("fork2");
            }else if(pid2 == 0){
                int read_res = read(s2, &buf, 1);
                if(read_res == -1) die("read");
                command(buf, s2);
            }else{
                c = getchar();
                int write_res = write(s2, &c, 1);
                if(write_res == -1) die("write");
                if(c == 'j') janken(s2);
            }
        }
    }else{
        communicate(s, cmd); // 通信するところ
    }
    
    if(argc == 2) close(ss);
    close(s2);

    return 0;
}
