#include "functions.h"

int clientfd = -1;

void catch_signal() {
    printf("\nSocket %d closed\n", clientfd);
    fflush(stdout);
    exit(close(clientfd));
}

int main() {
    signal(SIGINT, catch_signal);
    // read and process config file
    BotConfig conf;
    load_config("bot.cfg", &conf);
    printf("Configured sucesfully: %s:%d, nickname: %s, realname - %s\nChannels: %d\n",conf.server_ip, conf.port, conf.nick, conf.realname, conf.chan_num);
    for (int i = 0; i < conf.chan_num; ++i) {
        printf("%d => %s\n", i, conf.channels[i]);
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
    ignore_motd(clientfd);

    snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "JOIN #Unix\r\n");
    send(clientfd, msg_reg, strlen(msg_reg), 0);
    int valread = read(clientfd, msg_reg, IRC_MSG_BUFF_SIZE);
    msg_reg[valread] = '\0';
    printf("%s\n", msg_reg);
    free(msg_reg);

    char rec_buffer[IRC_MSG_BUFF_SIZE];
    int sock_rec_size;
    // ric_message parsed_msg;
    // int parentpid = getpid();
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
   

   

    return close(clientfd);
}