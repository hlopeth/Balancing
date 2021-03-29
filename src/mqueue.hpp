#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <cassert>
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

int msq_id = -1;

const int MAX_MSG_SIZE = 256;
struct my_msgbuf {
    long mtype;
    char mtext[MAX_MSG_SIZE];
};

bool create_mq() {
    key_t key = 10;
    msq_id = msgget(key, IPC_CREAT | 0666);
    std::cout << "Message queue created with id " << msq_id << std::endl;
    if(msq_id == -1) {
        std::cout << "create_mq ERROR: " << strerror(errno) << std::endl;
    }
    return msq_id != -1;
}

void send_msg(int type, std::string msg) {
    assert(msg.size() < MAX_MSG_SIZE);
    my_msgbuf buf = {
        .mtype = type
    };
    strcpy(buf.mtext, msg.c_str());
    auto res = msgsnd(msq_id, &buf, MAX_MSG_SIZE, 0);
    if(res == -1) {
        std::cout << "send_msg ERROR: " << strerror(errno) << std::endl;
    };
}

std::string recv_msg(int type) {
    my_msgbuf buf;
    auto res = msgrcv(msq_id, &buf, MAX_MSG_SIZE, type, 0);
    if(res == -1) {
        std::cout << "recv_msg ERROR: " << strerror(errno) << std::endl;
        return "";
    };
    std::string res_str(buf.mtext);
    return res_str;
}