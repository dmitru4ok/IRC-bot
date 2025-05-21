#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#define BUFF_SIZE 512

typedef struct {
    char server_ip[64];
    char nick[12];
    char user[12];
    char realname[64];
    char channels[512];
    int port;

} BotConfig;

void load_config(const char* filename, BotConfig* config) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Could not open config file");
        exit(1);
    }

    int max_buff = 200, read_len;
    char line_buff[max_buff];

    while (fgets(line_buff, sizeof(line_buff), f)) {
        read_len = strlen(line_buff);
        if (read_len == 0 || line_buff[0] == '#' ) continue;

        if (line_buff[read_len-1] == '\n' || line_buff[read_len-1] == '\r') {
            line_buff[read_len-1] = '\0';
        }

        char* equal_sign = strchr(line_buff, '=');
        if (!equal_sign) continue;
        char* val = equal_sign+1;
        *equal_sign = '\0';

        if (strcmp(line_buff, "server") == 0) {
            strncpy(config->server_ip, val, sizeof(config->server_ip)-1);
        } else if (strcmp(line_buff, "port") == 0) {
            config->port = atoi(val);
        } else if (strcmp(line_buff, "nick") == 0) {
            config->nick[0] = 'b';
            strncpy(config->nick + 1, val, sizeof(config->nick)-1);
        } else if (strcmp(line_buff, "user") == 0) {
            strncpy(config->user, val, sizeof(config->user)-1);
        } else if (strcmp(line_buff, "realname") == 0) {
            strncpy(config->realname, val, sizeof(config->realname)-1);
        } else if (strcmp(line_buff, "channels") == 0) {
            strncpy(config->channels, val, sizeof(config->channels)-1);
        }
    }

    fclose(f);
}

int main() {
    // read and process config file
    BotConfig conf;
    load_config("/home/yudm1317/unix25task3/bot.cfg", &conf);
    printf("Configured sucesfully: %s:%d, nickname: %s, realame - %s\n",conf.server_ip, conf.port, conf.nick, conf.realname);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0), status = 0;

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
    int valread = read(clientfd, buffer, BUFF_SIZE-1);
    buffer[valread] = '\0';
    printf("%s\n", buffer);

    close(clientfd);

    return 0;
}