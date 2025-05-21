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

#define BUFF_SIZE 512

typedef struct {
    char server_ip[64];
    char nick[12];
    char user[12];
    char realname[64];
    char channels[512];
    int port;

} BotConfig;

void load_config(const char* filename, BotConfig* config);
void catch_sigint_signal();
