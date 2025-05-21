#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>

#define IRC_MSG_BUFF_SIZE 513 // 512 for protocol + 1 for \0
#define MAX_PARAMS 15

typedef struct {
    char server_ip[64];
    char nick[12];
    char user[12];
    char realname[64];
    char channels[512];
    int port;

} BotConfig;

typedef struct {
    char prefix[20];
    char command[20];
    char params[MAX_PARAMS][480];
    int param_count;
} ric_message;

void load_config(const char* filename, BotConfig* config);
int parse_message(char* message, ric_message* out);
