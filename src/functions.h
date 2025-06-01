#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <linux/limits.h>
#include <time.h>

#define IRC_MSG_BUFF_SIZE 513 // 512 for protocol + 1 for \0
#define MAX_PARAMS 15
#define MAX_CHANNELS 10
#define CHANNEL_NAME_SIZE 201

typedef struct {
    char server_ip[64];
    char nick[12];
    char user[12];
    char realname[64];
    char logfile[PATH_MAX];
    char channels[MAX_CHANNELS][CHANNEL_NAME_SIZE];
    int chan_num;
    int port;
    int logs;

} BotConfig;

typedef struct {
    char prefix[200];
    char command[20];
    char params[MAX_PARAMS][200];
    int param_count;
} irc_message;

extern int main_to_children_pipes[MAX_CHANNELS][2];
extern int children_to_main_pipes[MAX_CHANNELS][2];
extern pid_t child_pids[MAX_CHANNELS];
extern BotConfig conf;
extern sem_t* sync_logging_sem;
extern pid_t parent;

void load_config(const char* filename, BotConfig* config);
int parse_message(char* message, irc_message* out);
int ignore_big_msg(int sockfd);
int connect_to_server(BotConfig* conf);
int server_reg(int,BotConfig*);
void listen_main(int);
void listen_child(int);
void join_channels(int);
void cleanup_main();
int find_channel_index(BotConfig*,char*);
int write_log(char*,char*);
void init_log(char*);
