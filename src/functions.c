#include "functions.h"

void load_config(const char* filename) {
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
            strncpy(conf->server_ip, val, sizeof(conf->server_ip)-1);
        } else if (strcmp(line_buff, "port") == 0) {
            conf->port = atoi(val);
        } else if (strcmp(line_buff, "nick") == 0) {
            conf->nick[0] = 'b';
            strncpy(conf->nick + 1, val, sizeof(conf->nick)-1);
        } else if (strcmp(line_buff, "user") == 0) {
            strncpy(conf->user, val, sizeof(conf->user)-1);
        } else if (strcmp(line_buff, "realname") == 0) {
            strncpy(conf->realname, val, sizeof(conf->realname)-1);
        } else if (strcmp(line_buff, "logfile") == 0) {
            strncpy(conf->logfile, val, sizeof(conf->realname)-1);
        } else if (strcmp(line_buff, "enable_logs") == 0) {
            conf->logs = atoi(val);
        } else if (strcmp(line_buff, "admin_channel") == 0) {
            strncpy(conf->admin_channel, val, sizeof(conf->admin_channel) - 1);
        } else if (strcmp(line_buff, "admin_channel_pass") == 0) {
            strncpy(conf->admin_pass, val, sizeof(conf->admin_pass) - 1);
        } else if (strcmp(line_buff, "narratives") == 0) {
            parse_narratives(val);
        } else if (strcmp(line_buff, "channels") == 0) {
            conf->chan_num = 0;
            char *iter = strtok(val, ",");
            while (iter != NULL) {
                if (conf->chan_num >= MAX_CHANNELS) {
                    break;
                }
                int len = strlen(iter);
                strncpy(conf->channels[conf->chan_num], iter, CHANNEL_NAME_SIZE - 1);
                conf->channels[conf->chan_num][len] = '\0';
                conf->chan_num++;
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

    char log_buff[IRC_MSG_BUFF_SIZE];
    write_log(conf->logfile, "---------\n");
    snprintf(log_buff, IRC_MSG_BUFF_SIZE, "%s|%s|%d\n", out->prefix, out->command, out->param_count);
    write_log(conf->logfile, log_buff);
    for (int i = 0; i < out->param_count; ++i) {
        snprintf(log_buff, IRC_MSG_BUFF_SIZE, "param[%d]:|%s\n", i, out->params[i]);
        write_log(conf->logfile, log_buff);
    }
    write_log(conf->logfile, "---------\n");
    return 0;
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
        write_log(conf->logfile, "Couldn't open socket!\n");
        cleanup_main();
    }
    

    struct sockaddr_in serv_addr;
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(conf->port);
    if(inet_pton(AF_INET, conf->server_ip, &serv_addr.sin_addr) < 0) {
        write_log(conf->logfile, "Couldn't convert IP!\n");
        cleanup_main();
    }

    if ((status = connect(clientfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
        write_log(conf->logfile, "Connection failed!\n");
        cleanup_main();
    }

    char success_buff[128];
    snprintf(success_buff, 128, "Connected on socket fd(%d)\n", clientfd);
    write_log(conf->logfile, success_buff);
    return clientfd;
}

void listen_main(int socket) {
    char rec_buff[IRC_MSG_BUFF_SIZE * 8];
    char resp_buff[IRC_MSG_BUFF_SIZE];
    char log_buff[IRC_MSG_BUFF_SIZE * 2];
    int len_rec = 0, regex_init = 1;
    irc_message msg;
    regex_t regex;
    const char *pattern = "^b[A-Za-z]{4}[0-9]{4}$";
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        write_log(conf->logfile, "Could not compile regex!");
        regex_init = 0;
    }

    while((len_rec = read(socket, rec_buff, IRC_MSG_BUFF_SIZE * 8)) > 0) {
        char* tmp = rec_buff;
        for (int j = 0; j < len_rec; ++j) {
            if (rec_buff[j] == '\n' && j > 0 && rec_buff[j-1] == '\r') { // message present
                rec_buff[j-1] = '\0';
                snprintf(log_buff, IRC_MSG_BUFF_SIZE * 2, "MESSAGE(%d): %s\n",  (int)strlen(tmp), tmp);
                write_log(conf->logfile, log_buff);
                parse_message(tmp, &msg);
                tmp = rec_buff+j+1;

                if (strcmp(msg.command, "PING") == 0) {
                    snprintf(resp_buff, IRC_MSG_BUFF_SIZE, "PONG %s\r\n", msg.params[0]);
                    write(socket, resp_buff, 7 + strlen(msg.params[0]));
                } else if (strcmp(msg.command, "PRIVMSG") == 0) {
                    int channel_index, len;
                    char sender[200];
                    if (strcmp(msg.params[0], conf->admin_channel) == 0) {
                        handle_admin_commands(socket, &msg);
                    } else if (
                        msg.param_count > 0 && 
                        regex_init &&
                        parse_user_from_prefix(msg.prefix, sender) == 0 &&
                        regexec(&regex, sender, 0, NULL, 0) != 0 &&    
                        ( channel_index = find_channel_index(msg.params[0])) >= 0
                    ) {
                        write(main_to_children_pipes[channel_index][1], &msg, sizeof(irc_message));
                        read(children_to_main_pipes[channel_index][0], &len, sizeof(int));
                        read(children_to_main_pipes[channel_index][0], &resp_buff, len);
                    }
                }
            }
        }
    }
    regfree(&regex);
}

void listen_child(int channel_no) {
    irc_message message;
    char resp[IRC_MSG_BUFF_SIZE];
    char sender[200];
    while ( read( main_to_children_pipes[channel_no][0], &message, sizeof(irc_message) ) > 0) {
        write_log(conf->logfile, "MESSAGE RECEIVED!\n");
        parse_user_from_prefix(message.prefix, sender);
        int len = snprintf(resp, IRC_MSG_BUFF_SIZE, "PRIVMSG %s :Hi %s!!! My topic: %s\r\n", message.params[0], sender, conf->channels[channel_no]);
        write(children_to_main_pipes[channel_no][1], &len, sizeof(int));
        write_log(conf->logfile, resp);
        write(children_to_main_pipes[channel_no][1], &resp, len);
    }
}

int server_reg(int socket) { // send NICK and USER messages, expect MOTD
    char msg_reg[IRC_MSG_BUFF_SIZE];
    snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "NICK %s\r\n", conf->nick);
  
    send(socket, msg_reg, strlen(msg_reg), 0);
    write_log(conf->logfile, msg_reg);
    snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "USER %s localhost * :%s\r\n", conf->user, conf->realname);
    send(socket, msg_reg, strlen(msg_reg), 0);
    write_log(conf->logfile, msg_reg);
    return 0;
}

void join_channels(int sock) {
    char join_msg[IRC_MSG_BUFF_SIZE] = "JOIN ";
    int offset = strlen(join_msg);
    for (int i = 0; i < conf->chan_num; ++i) {
        int written = snprintf(join_msg + offset, IRC_MSG_BUFF_SIZE - offset, "%s%s", 
            conf->channels[i], i < conf->chan_num - 1 ? "," : "\r\n");
        if (written < 0 || written >= IRC_MSG_BUFF_SIZE - offset) {
            fprintf(stderr, "Buffer overflow or error\n");
            cleanup_main();
        }
        offset += written;
    }
    send(sock, join_msg, strlen(join_msg), 0);
    write_log(conf->logfile, join_msg);
}

int find_channel_index(char* ch_name) {
    for (int ind = 0; ind < conf->chan_num; ++ind) {
        if (strcmp(conf->channels[ind], ch_name) == 0) {
            return ind;
        }
    }
    return -1;
}

void init_log(char* logfile) {
    if (!conf->logs) return;
    FILE *f = fopen(conf->logfile, "w");
    if (f == NULL) {
        printf("Failed to owverwrite log file!\n");
        exit(1);
    }
    fclose(f);
}

int write_log(char* logfile,char* msg) {
    if (!conf->logs) return 0;

    sem_wait(sync_logging_sem);
    FILE* f = fopen(logfile, "a");
    if (!f) {
        printf("Could not open the log file %s!\n", logfile);
        sem_post(sync_logging_sem);
        exit(1);
    }

    time_t rawtime;
    struct tm *timeinfo;
    char time_buff[64];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", timeinfo);
    fprintf(f, "%s - [%s] [%d] %s", getpid()  == parent ? "MAIN " : "CHILD", time_buff, getpid(), msg);
    fclose(f);

    sem_post(sync_logging_sem);
    return 0;

}

int parse_user_from_prefix(char* prefix, char* out) {
    char copy[200];
    snprintf(copy, sizeof(copy), "%s", prefix);
    char* exclamation = strchr(copy, '!');
    char* server_del = strchr(copy, '@');
    if (!exclamation || !server_del) {
        return -1;
    }
    *server_del = '\0';
    strcpy(out, exclamation+2);
    return 0;
}

void join_admin(int socket) {
    char msg[IRC_MSG_BUFF_SIZE];
    int l = snprintf(msg, IRC_MSG_BUFF_SIZE, "JOIN %s %s\r\n", conf->admin_channel, conf->admin_pass);
    write_log(conf->logfile, msg);
    write(socket, msg, l);
    l = snprintf(msg, IRC_MSG_BUFF_SIZE, "MODE %s +k %s\r\n", conf->admin_channel, conf->admin_pass);
    write_log(conf->logfile, msg);
    write(socket, msg, l);
}

void handle_admin_commands(int socket, irc_message* msg) {
    char* quit = "quit";
    char* switch_ = "switch";
    if (strncmp(msg->params[1], quit,  strlen(quit)) == 0) {
        char* msg = "QUIT :I must obey\r\n";
        write(socket, msg, strlen(msg));
    } else if (strncmp(msg->params[1], switch_, strlen(switch_)) == 0) {
        // switch <channel> <new narrative text>
        char copy[300], channel[CHANNEL_NAME_SIZE], new_narrative[30];
        strncpy(copy, msg->params[1], 300);
        char* space = strchr(copy, ' ');
        space++;
        char* iter = space;
        space = strchr(iter, ' ');   
        *space = '\0';
        strncpy(channel, iter, sizeof(channel));
        space++;
        iter = space;
        int i = 0;
        for (; i < conf->chan_num; ++i) {
            if (strcmp(channel, conf->channels[i]) == 0) {
                strncpy( conf->narratives[i], iter, sizeof(new_narrative));
                break;
            }
        }

        printf("change of narrative: %s -> %s\n", conf->channels[i], conf->narratives[i]);
    }
}

void parse_narratives(char* val) {
    int looper = 0;
    char *iter = strtok(val, ",");
    while (iter != NULL) {
        if (looper >= MAX_CHANNELS) {
            break;
        }
        int len = strlen(iter);
        strncpy(conf->narratives[looper], iter, CHANNEL_NAME_SIZE - 1);
        conf->narratives[looper][len] = '\0';
        // printf("Narrative: %s\n", conf->narratives[looper]);
        looper++;
        iter = strtok(NULL, ",");
    }
}