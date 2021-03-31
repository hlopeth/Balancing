#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <unistd.h>

#include "mqueue.hpp"
#include "job.hpp"
#include "json.hpp"
using json = nlohmann::json;

struct t_procces {
    int id;
    std::vector<int> out;
};


std::vector<t_procces> read_topology(std::string);

//procecces data
t_procces pd;
int my_mq_id;
std::vector<int> out_mq_ids;


std::list<t_job> jobs;


int main() {
    std::vector<t_procces> processes = read_topology("topology/simple_tree.json");

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
    my_mq_id = create_mq(pd.id);
    for(auto it: pd.out) {
        auto mq_id = create_mq(it);
        out_mq_ids.push_back(mq_id);
        //send_msg(mq_id, 1, "Fuck you num " + std::to_string(it));
    }
    // for(int it: pd.out) {
    //     std::cout << "My id " << pd.id << ". I got \"" << recv_msg(my_mq_id, 1) << "\" =(" << std::endl;
    // }

    while (true)
    {
        // check_msg();
        //try_initiate_rebalance();
        auto job = generate_job(0.3);
        if(job != NULL_JOB) {
            jobs.push_back(job);
        }
        if(test_rate(0.2)) {
            jobs.pop_front();
        }

        float load = get_load(jobs);
        if(pd.id == 1) {
            std::cout << "\r load " << load / 100 << "%\n";
        }

        sleep(1);
    }
    
    return 0;
}

std::vector<t_procces> read_topology(std::string filename) {
    std::vector<t_procces> result;
    std::ifstream i(filename.c_str());
    json j;
    i >> j;
    int size = j["size"].get<int>();
    for(int i = 0; i < size; i++) {
        t_procces descr;
        descr.id = j["processes"][i]["id"].get<int>();
        auto out = j["processes"][i]["to"];
        for(auto element: out) {
            descr.out.push_back(element.get<int>());
        }
        result.push_back(descr);
    }
    return result;
}