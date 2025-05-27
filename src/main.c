#include "functions.h"

int clientfd = -1;
int pipesfd[MAX_CHANNELS][2];
int procids[MAX_CHANNELS] = { -1 };
BotConfig conf;

void catch_signal() {
    printf("\nSocket %d closed\n", clientfd);
    fflush(stdout);
    for (int i = 0; i < conf.chan_num; ++i) {
        close(pipesfd[i][0]);
        close(pipesfd[i][1]);
    }
    exit(close(clientfd));
}

int main() {
    signal(SIGINT, catch_signal);
    // read and process config file
    load_config("bot.cfg", &conf);

    printf("Configured sucesfully: %s:%d, nickname: %s, realname - %s\nChannels: %d\n",conf.server_ip, conf.port, conf.nick, conf.realname, conf.chan_num);
    for (int i = 0; i < conf.chan_num; ++i) {
        if (pipe(pipesfd[i]) == -1) {
            printf("Opening a pipe failed\n");
            return 1;
        }
        // printf("pipe[%d][0] = %d, pipe[%d][1] = %d   ", i, pipesfd[i][0], i, pipesfd[i][1]);
        // printf("%d => %s\n", i, conf.channels[i]);
    }

    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    int status = 0;

    if (clientfd < -1) {
        perror("Socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(conf.port);
    if(inet_pton(AF_INET, conf.server_ip, &serv_addr.sin_addr) < 0) {
        perror("IP addr");
        exit(1);
    }

    if ((status = connect(clientfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
        perror("Connection failed");
        exit(1);
    }

    char* msg_reg = malloc(IRC_MSG_BUFF_SIZE);
    snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "NICK %s\r\n", conf.nick);
    send(clientfd, msg_reg, strlen(msg_reg), 0);
    snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "USER %s localhost * :%s\r\n", conf.user, conf.realname);
    send(clientfd, msg_reg, strlen(msg_reg), 0); // send NICK and USER messages, expect MOTD
    ignore_big_msg(clientfd); // motd

    int childpid = 0, ch_no=0;
    for (ch_no = 0; ch_no < conf.chan_num; ++ch_no) {
        snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "JOIN %s\r\n", conf.channels[ch_no]);
        send(clientfd, msg_reg, strlen(msg_reg), 0);
        ignore_big_msg(clientfd);
        childpid = fork();
        if (childpid == 0) {
            break;
        } else if (childpid == -1) {
            printf("Failed to create new proccess!\n");
            return 1;
        }
    }
    free(msg_reg);

    if (childpid == 0) {
        // add signal to catch termination from a parent
        for (int i = 0; i < conf.chan_num; ++i) {
            close(pipesfd[i][1]);
            if (ch_no != i) {
                close(pipesfd[i][0]);
            }
        }

        char ch_name[CHANNEL_NAME_SIZE];
        read(pipesfd[ch_no][0], ch_name, CHANNEL_NAME_SIZE);
        printf("I am child %d, my channel name is %s\n", ch_no, ch_name);
        close(pipesfd[ch_no][0]);
        printf("Child %d (%d) exiting\n", ch_no, getpid());

        return 0;
    } else {
        // close pipe reading channels
        for (int i = 0; i < conf.chan_num; ++i) {
            close(pipesfd[i][0]);
            printf("Piping %s into process\n", conf.channels[i]);
            write(pipesfd[i][1], conf.channels[i], sizeof(conf.channels[i]));
        }
        int pid_died;

        printf("\n\n\nMAIN: done sending, now waiting for child processes!\n\n\n");
        while((pid_died = wait(NULL)) != -1 || errno != ECHILD) {
            printf("MAIN(%d): waited for a child %d to die\n", getpid(), pid_died);
        };

         // close pipe writing channels
        for (int i = 0; i < conf.chan_num; ++i) {
            close(pipesfd[i][1]);
        }
        return 0;
    }

   
    
    
    // int valread = read(clientfd, msg_reg, IRC_MSG_BUFF_SIZE);
    // msg_reg[valread] = '\0';
    // printf("%s\n", msg_reg);

    char rec_buffer[IRC_MSG_BUFF_SIZE];
    int sock_rec_size;
    // ric_message parsed_msg;
    
    while ((sock_rec_size = recv(clientfd, rec_buffer, IRC_MSG_BUFF_SIZE, 0)) > 0) {
        printf("%s", rec_buffer);
    //     rec_buffer[sock_rec_size] = '\0';
    //     char* begin = rec_buffer;
    //     char* end = strstr(rec_buffer, "\r\n");
    //     while (end != NULL) { // untill full lines keep coming
    //         printf("|||PARSING: %s\n", begin);
    //         *end ='\0';
    //         begin = end + 2;
    //         parse_message(motd_parse_ptr, &parsed_msg); // parse them
    //         end = strstr(begin, "\r\n"); // get next one
    //     }
        
    //     // if (strstr(rec_buffer, " 376 ") || strstr(rec_buffer, " 422 ")) {
    //     //     break;
    }

    // } // discard MOTD
    printf("Error happened!");


    for (int i = 0; i < conf.chan_num; ++i) {
        close(pipesfd[i][0]);
        close(pipesfd[i][1]);
    }
   
    return close(clientfd);
}