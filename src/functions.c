#include "functions.h"

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
            config->chan_num = 0;
            char *iter = strtok(val, ",");
            while (iter != NULL) {
                if (config->chan_num >= MAX_CHANNELS) {
                    break;
                }
                int len = strlen(iter);
                printf("%d => %s\n", len, iter);
                strncpy(config->channels[config->chan_num], iter, CHANNEL_NAME_SIZE - 1);
                config->channels[config->chan_num][len] = '\0';
                config->chan_num++;
                iter = strtok(NULL, ",");
            }
        }
    }

    fclose(f);
}

int parse_message(char *message, irc_message* out) {
    if (!message) return -1;
    int len = strlen(message);
    if (len <= 0 || len > IRC_MSG_BUFF_SIZE) return -1;

    char copy_local[IRC_MSG_BUFF_SIZE];
    strncpy(copy_local, message, IRC_MSG_BUFF_SIZE);

    char* iter = copy_local;
    if (copy_local[0] == ':') {
        iter = strchr(copy_local+1, ' ');
        *iter = '\0';
        strcpy(out->prefix, copy_local+1);
        iter++;
    } else {
        out->prefix[0]='\0';
    }

    while (*iter == ' ') iter++;
    char* term = strchr(iter, ' ');
    *term = '\0';
    strcpy(out->command, iter);
    while (*term == ' ' || *term == '\0') term++;
    iter = term;

    // params
    out->param_count = 0;
    while (*iter == ':' || (term = strchr(iter, ' '))) {
        printf("Iteration: %d, left:|%s\n", out->param_count, iter);
        if (*iter == ':') {
            strncpy(out->params[out->param_count], iter+1 , 512);
            out->param_count += 1;
            break;
        } else {
            *term = '\0';
            strncpy(out->params[out->param_count], iter , 200);
        }

        out->param_count += 1;
        term += 1;
        while (*term == ' ') ++term;
        iter = term;
    }
}

int ignore_big_msg(int sockfd) {
    const int size = 8196;
    char waste[size];
    int res = recv(sockfd, waste, size, MSG_DONTWAIT);
    waste[res] = '\0';
    printf("%s", waste);

    return res;
}

int connect_to_server(BotConfig* conf) {
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    int status = 0;

    if (clientfd < -1) {
        perror("Socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(conf->port);
    if(inet_pton(AF_INET, conf->server_ip, &serv_addr.sin_addr) < 0) {
        perror("IP addr");
        exit(1);
    }

    if ((status = connect(clientfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
        perror("Connection failed");
        exit(1);
    }
    return clientfd;
}

void listen_main(int socket) {
    char rec_buff[IRC_MSG_BUFF_SIZE * 2];
    int i = 0, len_rec = 0;

    while((len_rec = read(socket, rec_buff, IRC_MSG_BUFF_SIZE * 2)) > 0) {
        rec_buff[len_rec] = 0;
        printf("%s", rec_buff);
        printf("Piping %s into process\n", conf.channels[i % conf.chan_num]);
        write(main_to_children_pipes[i][1], conf.channels[i % conf.chan_num], strlen(conf.channels[i % conf.chan_num]) + 1);
        ++i;
    }

    int pid_died;
    printf("\n\n\nMAIN: done sending, now waiting for child processes!\n\n\n");
    while((pid_died = wait(NULL)) != -1 || errno != ECHILD) {
        printf("MAIN(%d): waited for a child %d to die\n", getpid(), pid_died);
    };
   

    // close pipe writing channels
    for (int i = 0; i < conf.chan_num; ++i) {
        close(main_to_children_pipes[i][1]);
    }
    sem_destroy(sync_logging_sem);
    munmap(sync_logging_sem, sizeof(sem_t));
}

void listen_child(int socket, int channel_no) {
    for (int i = 0; i < conf.chan_num; ++i) {
        close(main_to_children_pipes[i][1]);
        if (channel_no != i) {
            close(main_to_children_pipes[i][0]);
        }
    }

    char ch_name[CHANNEL_NAME_SIZE];
    char resp_buff[IRC_MSG_BUFF_SIZE];
    while (read(main_to_children_pipes[channel_no][0], ch_name, CHANNEL_NAME_SIZE) > 0) {
        snprintf(resp_buff, IRC_MSG_BUFF_SIZE, "PART %s\r\n", ch_name);
        write(socket, resp_buff, strlen(resp_buff));
        printf("I am child %d, my channel name is %s\n", channel_no, ch_name);
    }
   
    close(main_to_children_pipes[channel_no][0]);
    printf("Child %d (%d) exiting\n", channel_no, getpid());
    munmap(sync_logging_sem, sizeof(sem_t));
}

int server_reg(int socket, BotConfig *conf) { // send NICK and USER messages, expect MOTD
    char msg_reg[IRC_MSG_BUFF_SIZE];
    snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "NICK %s\r\n", conf->nick);
    // printf("%s", msg_reg);
    send(socket, msg_reg, strlen(msg_reg), 0);
    snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "USER %s localhost * :%s\r\n", conf->user, conf->realname);
    // printf("%s", msg_reg);
    send(socket, msg_reg, strlen(msg_reg), 0);
    return 0;
}