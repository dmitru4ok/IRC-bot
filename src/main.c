#include "functions.h"

int clientfd = -1;
int pipesfd[MAX_CHANNELS][2];
pid_t child_pids[MAX_CHANNELS];
BotConfig conf;
sem_t* sync_write;
pid_t parent;

void cleanup_main() {
    // if (getpid() != parent) return;
    printf("%d is preforming a cleanup!\n", getpid());
    fflush(stdout);
    for (int i = 0; i < conf.chan_num; ++i) { //pipes
        close(pipesfd[i][0]);
        close(pipesfd[i][1]);
    }
    sem_destroy(sync_write); // semafore
    munmap(sync_write, sizeof(sem_t)); //shared memory
    close(clientfd);
    exit(0);
}

int main() {
    signal(SIGINT, cleanup_main);
    sync_write = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    sem_init(sync_write, 1, 1);
    parent = getpid();
    // read and process config file
    load_config("bot.cfg", &conf);

    printf("Configured sucesfully: %s:%d, nickname: %s, realname - %s\nChannels: %d\n",conf.server_ip, conf.port, conf.nick, conf.realname, conf.chan_num);
    for (int i = 0; i < conf.chan_num; ++i) {
        if (pipe(pipesfd[i]) == -1) {
            fprintf(stderr, "Opening a pipe failed\n");
            return 1;
        }
    }
    clientfd = connect_to_server(&conf);

    char* msg_reg = malloc(IRC_MSG_BUFF_SIZE);
    snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "NICK %s\r\n", conf.nick);
    send(clientfd, msg_reg, strlen(msg_reg), 0);
    snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "USER %s localhost * :%s\r\n", conf.user, conf.realname);
    send(clientfd, msg_reg, strlen(msg_reg), 0); // send NICK and USER messages, expect MOTD
    ignore_big_msg(clientfd); // motd

    int ch_no = 0, childpid = 0;
    for (ch_no = 0; ch_no < conf.chan_num; ++ch_no) {
        snprintf(msg_reg, IRC_MSG_BUFF_SIZE, "JOIN %s\r\n", conf.channels[ch_no]);
        send(clientfd, msg_reg, strlen(msg_reg), 0);
        ignore_big_msg(clientfd);
        childpid = fork();

        child_pids[ch_no] = childpid;
        if (childpid == 0) {
            child_pids[ch_no] = 0;
            break;
        } else if (childpid == -1) {
            printf("Failed to create new proccess!\n");
            return 1;
        }
    }
    free(msg_reg);

    if (childpid == 0) {
        // CHILD WORKING PROCESSES
        listen_child(ch_no);
        return 0;
    } else { 
        // MAIN
        listen_main();
        return 0;
    }

    printf("Error happened!");
    return close(clientfd);
}