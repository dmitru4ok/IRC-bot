#include "functions.h"

int clientfd = -1;

void catch_signal() {
    printf("\nSocket %d closed\n", clientfd);
    exit(close(clientfd));
}

int main() {
    signal(SIGINT, catch_signal);
    // read and process config file
    BotConfig conf;
    load_config("bot.cfg", &conf);
    printf("Configured sucesfully: %s:%d, nickname: %s, realame - %s\n",conf.server_ip, conf.port, conf.nick, conf.realname);

    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    int status = 0;

    if (clientfd < -1) {
        perror("Socket");
        exit(1);
    }

    char buffer[BUFF_SIZE];
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
    char msg_nick[BUFF_SIZE]; 
    char msg_user[BUFF_SIZE];
    snprintf(msg_nick, BUFF_SIZE-2, "NICK %s\r\n", conf.nick);
    snprintf(msg_user, BUFF_SIZE-2, "USER %s localhost * :%s\r\n", conf.user, conf.realname);
    send(clientfd, msg_nick, strlen(msg_nick), 0);
    send(clientfd, msg_user, strlen(msg_user), 0);
    read(clientfd, buffer, BUFF_SIZE-1);
    read(clientfd, buffer, BUFF_SIZE-1);
    read(clientfd, buffer, BUFF_SIZE-1);
    int valread = read(clientfd, buffer, BUFF_SIZE-1);
    buffer[valread] = '\0';
    printf("%s\n", buffer);

    return close(clientfd);
}