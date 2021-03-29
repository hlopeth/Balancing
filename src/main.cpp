#include <iostream>
#include <fstream>
#include <vector>

#include "mqueue.hpp"
#include "json.hpp"
using json = nlohmann::json;

struct p_descr {
    int id;
    std::vector<int> out;
};

std::vector<p_descr> read_topology(std::string);

p_descr pd;

int main() {

    if(!create_mq()) {
        return 0;
    }
    std::vector<p_descr> processes = read_topology("topology/tree.json");

    //создание процессов
    pd = processes[0];
    pid_t pid;
    for (int i = 1; i < processes.size(); i++)
    {
        pid = fork();
        if(pid != 0) {
            pd = processes[i];
            break;
        }
    }

    //общий код
    for(auto it: pd.out) {
        send_msg(it, "Fuck my num " + std::to_string(it));
    }
    for(int it: pd.out) {
        std::cout << "My id " << pd.id << ". I got \"" << recv_msg(pd.id) << "\" =(" << std::endl;
    }
    return 0;
}

std::vector<p_descr> read_topology(std::string filename) {
    std::vector<p_descr> result;
    std::ifstream i(filename.c_str());
    json j;
    i >> j;
    int size = j["size"].get<int>();
    for(int i = 0; i < size; i++) {
        p_descr descr;
        descr.id = j["processes"][i]["id"].get<int>();
        auto out = j["processes"][i]["to"];
        for(auto element: out) {
            descr.out.push_back(element.get<int>());
        }
        result.push_back(descr);
    }
    return result;
}
