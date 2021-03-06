#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <cassert>
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

// #define DEBUG

const int MAX_N = 32;
const int msg_sz = (MAX_N + MAX_N + 2) * sizeof(int);
struct my_msg {
    long mtype;
    int ids[MAX_N];
    int jobs[MAX_N];
    int n;
    int from_id;
};
struct my_msgbuf {
    long mtype;
    char data[msg_sz];
};

const int move_sz = (MAX_N * 4 + 2 ) * sizeof(int);
struct move_t {
    long mtype;
    int from_id[MAX_N];
    int to_id[MAX_N];
    int jobs[MAX_N];
    int n;
    int visited_nodes[MAX_N];
    int m;
};
struct move_buff {
    long mtype;
    char data[move_sz];
};

const int finn_sz = (MAX_N * 3 + 2) * sizeof(int);
struct finn_t {
    long mtype;
    int inc[MAX_N];
    int n_inc[MAX_N];
    int jobs[MAX_N];
    int inc_size;
    int n_inc_size;
};
struct finn_buff {
    long mtype;
    int data[finn_sz];
};



int code(int num1, int num2) {
    int a = std::max(num1, num2);
    int b = std::min(num1, num2);
    return 0.5 * (a + b) * (a + b + 1) + b;
}

int create_mq(int key) {
    int msq_id = msgget(key, IPC_CREAT | 0666);
    // std::cout << "Message queue created with id " << msq_id << std::endl;
    #ifdef DEBUG
    if(msq_id == -1) {
        std::cout << "create_mq ERROR: " << strerror(errno) << std::endl;
    }
    #endif
    return msq_id;
}

void send_msg(int msq_id, my_msg msg) {
    my_msgbuf buf;
    memcpy(&buf, &msg, msg_sz + sizeof(long));
    auto res = msgsnd(msq_id, &buf, msg_sz, 0);
    #ifdef DEBUG
    if(res == -1) {     
        std::cout << "send_msg ERROR: " << strerror(errno) << " " << msq_id << std::endl;
    };
    #endif
}

my_msg recv_msg(int msq_id, int type) {
    my_msgbuf buf;
    auto res = msgrcv(msq_id, &buf, msg_sz, type, 0);
    #ifdef DEBUG
    if(res == -1) {
        std::cout << "recv_msg ERROR: " << strerror(errno) << std::endl;
    }
    #endif
    my_msg msg;
    memcpy(&msg, &buf, msg_sz + sizeof(long));
    return msg;
}

void send_move(int msq_id, move_t move) {
    move_buff buf;
    memcpy(&buf, &move, move_sz + sizeof(long));
    auto res = msgsnd(msq_id, &buf, move_sz, 0);
    #ifdef DEBUG
    if(res == -1) {     
        std::cout << "send_msg ERROR: " << strerror(errno) << " " << msq_id << std::endl;
    }
    #endif
}

move_t recv_move(int msq_id, int type) {
    move_buff buf;
    auto res = msgrcv(msq_id, &buf, move_sz, type, 0);
    #ifdef DEBUG
    if(res == -1) {
        std::cout << "recv_msg ERROR: " << strerror(errno) << std::endl;
    }
    #endif
    move_t move;
    memcpy(&move, &buf, move_sz + sizeof(long));
    return move;
}

void send_finn(int msq_id, finn_t finn) {
    finn_buff buf;
    memcpy(&buf, &finn, finn_sz + sizeof(long));
    auto res = msgsnd(msq_id, &buf, finn_sz, 0);
    #ifdef DEBUG
    if(res == -1) {     
        std::cout << "send_msg ERROR: " << strerror(errno) << " " << msq_id << std::endl;
    }
    #endif
}

finn_t recv_finn(int msq_id, int type) {
    finn_buff buf;
    auto res = msgrcv(msq_id, &buf, finn_sz, type, 0);
    #ifdef DEBUG
    if(res == -1) {
        std::cout << "recv_msg ERROR: " << strerror(errno) << std::endl;
    }
    #endif
    finn_t finn;
    memcpy(&finn, &buf, finn_sz + sizeof(long));
    return finn;
}