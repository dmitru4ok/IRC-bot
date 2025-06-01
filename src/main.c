#include "functions.h"

int clientfd = -1;
int main_to_children_pipes[MAX_CHANNELS][2];
int children_to_main_pipes[MAX_CHANNELS][2];
pid_t child_pids[MAX_CHANNELS];
BotConfig conf;
sem_t* sync_logging_sem;
pid_t parent;

void cleanup_main() {
    // only called by main, loop through child_pids to signal children to quit
    printf("\n%d is preforming a cleanup!\n", getpid());
    int pid_died;
    while((pid_died = wait(NULL)) != -1 || errno != ECHILD) {
        printf("MAIN(%d): waited for a child %d to die\n", getpid(), pid_died);
    };
    
   
    fflush(stdout);
    for (int i = 0; i < conf.chan_num; ++i) { //pipes
        close(main_to_children_pipes[i][0]);
        close(main_to_children_pipes[i][1]);
        close(children_to_main_pipes[i][0]);
        close(children_to_main_pipes[i][1]);
    }
    sem_destroy(sync_logging_sem); // semafore
    munmap(sync_logging_sem, sizeof(sem_t)); //shared memory
    close(clientfd);
    exit(0);
}

int main() {
    sync_logging_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    sem_init(sync_logging_sem, 1, 1);
    parent = getpid();
    // read and process config file
    load_config("bot.cfg", &conf);

    printf("Configured sucesfully: %s:%d, nickname: %s, realname - %s\nChannels: %d\n",conf.server_ip, conf.port, conf.nick, conf.realname, conf.chan_num);
    for (int i = 0; i < conf.chan_num; ++i) {
        if (pipe(main_to_children_pipes[i]) == -1 || pipe(children_to_main_pipes[i]) == -1) {
            fprintf(stderr, "Opening a pipe failed\n");
            return 1;
        }
    }

    signal(SIGINT, cleanup_main);

    int ch_no = 0, childpid = 0;
    for (ch_no = 0; ch_no < conf.chan_num; ++ch_no) {
        childpid = fork();
        child_pids[ch_no] = childpid;
        if (childpid == 0) {
            break;
        } else if (childpid == -1) {
            printf("Failed to create new proccess!\n");
            return 1;
        }
    }

    if (childpid == 0) {
        // CHILD WORKING PROCESSES

        for (int i = 0; i < conf.chan_num; ++i) {
            close(main_to_children_pipes[i][1]);
            close(children_to_main_pipes[i][0]);
            if (ch_no != i) {
                close(main_to_children_pipes[i][0]);
                close(children_to_main_pipes[i][1]);
            }
        }

        listen_child(ch_no);

        close(main_to_children_pipes[ch_no][0]);
        close(children_to_main_pipes[ch_no][1]);
        printf("Child %d (%d) exiting\n", ch_no, getpid());
        munmap(sync_logging_sem, sizeof(sem_t));
        return 0;
    } else {
        for (int i = 0; i < conf.chan_num; ++i) {
            close(main_to_children_pipes[i][0]);
            close(children_to_main_pipes[i][1]);
        }

        // open socket
        int clientfd = connect_to_server(&conf);
        // register
        server_reg(clientfd, &conf);
        //join channels

        while ( ignore_big_msg(clientfd) > 0); // janky, will work for now
        join_channels(clientfd);
        listen_main(clientfd);
       
        for (int i = 0; i < conf.chan_num; ++i) {
            close(main_to_children_pipes[i][1]);
            close(children_to_main_pipes[i][0]);
        }
        munmap(sync_logging_sem, sizeof(sem_t));
        return 0;
    }

    printf("Error happened!");
    cleanup_main();
    return close(clientfd);
}