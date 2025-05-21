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
            strncpy(config->channels, val, sizeof(config->channels)-1);
        }
    }

    fclose(f);
}