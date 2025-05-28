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
    printf("\n%d(main) is preforming a cleanup!\n", getpid());
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
    

    int ch_no = 0, childpid = 0;
    for (ch_no = 0; ch_no < conf.chan_num; ++ch_no) {
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

    if (childpid == 0) {
        // CHILD WORKING PROCESSES
        // listen_child(socket, ch_no);
        // irc_message rec_msg;
        int size;
        // char msg[IRC_MSG_BUFF_SIZE];
        while(read(main_to_children_pipes[ch_no][0], &size, sizeof(int)) > 0) {
            // snprintf(msg, IRC_MSG_BUFF_SIZE, "%s %s %s\r\n\0", rec_msg.command, rec_msg.params[0], rec_msg.params[1]);
            printf("%d: Received from main: %d\n", getpid(), size);
            fflush(stdout);
            sleep(2);

            int value = size + 1;
            printf("%d: Sending back to main: %d\n", getpid(), value);
            fflush(stdout);
            write(children_to_main_pipes[ch_no][1], &value, sizeof(int));
        }
        return 0;
    } else {
        // MAIN
        // signal(SIGINT, cleanup_main);
       
        for (int i = 0; i < conf.chan_num; ++i) {
            close(main_to_children_pipes[i][0]);
            close(children_to_main_pipes[i][1]);
        }
        //open socket
        // int clientfd = connect_to_server(&conf);
        //register
        // server_reg(clientfd, &conf);
        //join channels
        // char join_msg[IRC_MSG_BUFF_SIZE] = "JOIN ";
        // int offset = strlen(join_msg);
        // for (int i = 0; i < conf.chan_num; ++i) {
        //     int written = snprintf(join_msg + offset, IRC_MSG_BUFF_SIZE - offset, "%s%s", 
        //         conf.channels[i], i < conf.chan_num - 1 ? "," : "\r\n");
        //     if (written < 0 || written >= IRC_MSG_BUFF_SIZE - offset) {
        //         fprintf(stderr, "Buffer overflow or error\n");
        //         cleanup_main();
        //     }
        //     offset += written;
        //     // printf("String after %d: %s\n", i, join_msg);
        // }
        // send(clientfd, join_msg, strlen(join_msg), 0);
        // while ( ignore_big_msg(clientfd) > 0); // janky, will work for now
        char* ch_name = "#Unix";
        int i;

        // irc_message msg;
        char rec_buff[IRC_MSG_BUFF_SIZE];
        int value = 0, res;
        while (1 || recv(clientfd, rec_buff, IRC_MSG_BUFF_SIZE, 0) > 0) {
            for (i=0; i < conf.chan_num; ++i) {
                if (strcmp(ch_name, conf.channels[i])) break;
            }

            if (i == conf.chan_num) continue;
            // strcpy(msg.command, "PRIVMSG");
            // strcpy(msg.params[0], "#Unix");
            // strcpy(msg.params[1], ":Hi from a bot!");
            // msg.param_count = value;

            printf("Write to child: %d\n", value);
            fflush(stdout);
            write(main_to_children_pipes[i][1], &value, sizeof(int));
            read(children_to_main_pipes[i][0], &res, sizeof(int));
            printf("Received from child: %d\n", res);
            fflush(stdout);
            ++value;
        }
       

        


        


        // listen_main();
        return 0;
    }

    printf("Error happened!");
    return close(clientfd);
}