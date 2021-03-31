#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <cassert>
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

const int MAX_MSG_SIZE = 256;
struct my_msgbuf {
    long mtype;
    char mtext[MAX_MSG_SIZE];
};

int code(int num1, int num2) {
    int a = std::max(num1, num2);
    int b = std::min(num1, num2);
    return 0.5 * (a + b) * (a + b + 1) + b;
}

int create_mq(int key) {
    int msq_id = msgget(key, IPC_CREAT | 0666);
    std::cout << "Message queue created with id " << msq_id << std::endl;
    if(msq_id == -1) {
        std::cout << "create_mq ERROR: " << strerror(errno) << std::endl;
    }
    return msq_id;
}

void send_msg(int msq_id, int type, std::string msg) {
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

std::string recv_msg(int msq_id, int type) {
    my_msgbuf buf;
    auto res = msgrcv(msq_id, &buf, MAX_MSG_SIZE, type, 0);
    if(res == -1) {
        std::cout << "recv_msg ERROR: " << strerror(errno) << std::endl;
        return "";
    };
    std::string res_str(buf.mtext);
    return res_str;
}