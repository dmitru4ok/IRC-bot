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

int parse_message(char *message, ric_message* out) {
    if (!message) return -1;
    int len = strlen(message);
    if (len <= 0) return -1;

    char copy_local[IRC_MSG_BUFF_SIZE];
    strncpy(copy_local, message, IRC_MSG_BUFF_SIZE);
    // printf("PARSING: %s\n", copy_local);

    char* iter = copy_local;
    if (copy_local[0] == ':') { // prefix present
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
    // printf("    prefix: => %s, command => %s\n", out->prefix, out->command);
    return 0;

}

int ignore_big_msg(int sockfd) {
    char waste[4096];
    int res = read(sockfd, waste, 4096);
    // printf("%s", waste);

    return res;
}