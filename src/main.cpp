#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <unistd.h>

#include "mqueue.hpp"
#include "json.hpp"
using json = nlohmann::json;

#define ECHO_ID 1

struct t_procces {
    int id;
    int jobs;
    std::vector<int> out;
};

struct job_move {
    int from_id;
    int to_id;
    int jobs;
};

std::vector<t_procces> read_topology(std::string);
bool has(my_msg msg, int id);
void add_to_msg(my_msg &bud, int id, int jobs);
std::vector<job_move> generate_moves(my_msg &msg);

//procecces data
t_procces pd;
int my_mq_id;
std::map<int, int> id_to_mqId;

int main() {
    std::vector<t_procces> processes = read_topology("topology/tree.json");

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
        id_to_mqId[it] = create_mq(it);
    }

    std::cout << "my id " << pd.id << " my mq_id " << my_mq_id << std::endl;

    //инициализация балансировки нагрузи в root
    if(pid == 0) {
        //инициация балансировки
        my_msg msg;
        msg.mtype = ECHO_ID;
        msg.ids[0] = pd.id;
        msg.jobs[0] = pd.jobs;
        msg.n = 1;
        msg.from_id = pd.id;

        std::map<int, int> ids_to_jobs;
        //в очереди соседей отправляем сообщение
        for(auto id: pd.out) {
            // std::cout << "send from id " << msg.from_id << " to " << id << std::endl;
            send_msg(id_to_mqId[id], msg);
        }
        //получаем ответы в свою очередь
        for(auto id: pd.out) {
            my_msg res_msg = recv_msg(my_mq_id, ECHO_ID);
            // std::cout << "reciving in to " << pd.id << " from " <<  res_msg.from_id << std::endl;
            for(int i = 0; i < res_msg.n; i++) {
                if(!has(msg, res_msg.ids[i])) {
                    add_to_msg(msg, res_msg.ids[i], res_msg.jobs[i]);
                }
            }
        }
        //делаем что то с полученными данными
        std::cout << "results:" << std::endl;
        for(int i = 0; i < msg.n; i++) {
            std::cout << "id: " << msg.ids[i] << " job " << msg.jobs[i] << std::endl;
        }
        std::cout << "moves: " << std::endl;
        auto moves = generate_moves(msg);
        for(int i = 0 ; i < moves.size(); i++) {
            auto move = moves[i];
            std::cout << "from " << move.from_id << " to " << move.to_id << " jobs " << move.jobs << std::endl;
        }
        std::cout << "results:" << std::endl;
        for(int i = 0; i < msg.n; i++) {
            std::cout << "id: " << msg.ids[i] << " job " << msg.jobs[i] << std::endl;
        }
        
    } else {
        //получаем сообщение в свою очередь
        my_msg msg = recv_msg(my_mq_id, ECHO_ID);
        // std::cout << "first reciving in to " << pd.id << " from " << msg.from_id << std::endl;

        int from = msg.from_id;
        //добавляем свои данные
        msg.from_id = pd.id;
        add_to_msg(msg, pd.id, pd.jobs);

        //в очереди соседей которых нет в msg.ids отправляем сообщение
        int send_counter = 0;
        for(auto id: pd.out) {
            if(!has(msg, id)) {
                // std::cout << "send from id " << msg.from_id << " to " << id << std::endl;
                send_msg(id_to_mqId[id], msg);
                send_counter += 1;
            }
        }

        //получаем ответы в свою очередь и дополняем msg
        for (int i = 0; i < send_counter; i++)
        {
            my_msg res_msg = recv_msg(my_mq_id, ECHO_ID);
            // std::cout << "reciving in to " << pd.id << " from " <<  res_msg.from_id << std::endl;
            for (int i = 0; i < res_msg.n; i++)
            {
                if(!has(msg, res_msg.ids[i])) {
                    add_to_msg(msg, res_msg.ids[i], res_msg.jobs[i]);
                }
            }
        }
        
        //возвращаем msg
        send_msg(id_to_mqId[from], msg);
        // std::cout << "last send from id " << msg.from_id << " to " << from  << std::endl;
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
        descr.jobs = j["processes"][i]["jobs"].get<int>();
        auto out = j["processes"][i]["to"];
        for(auto element: out) {
            descr.out.push_back(element.get<int>());
        }
        result.push_back(descr);
    }
    return result;
}

bool has(my_msg msg, int id) {
    for(int i = 0; i < msg.n; i++) {
        if(msg.ids[i] == id) 
            return true;
    }
    return false;
}

void add_to_msg(my_msg &msg, int id, int jobs) {
        int n = msg.n;
        msg.ids[n] = id;
        msg.jobs[n] = jobs;
        msg.n += 1;
}


int get_or_put(std::vector<job_move> &moves, int from_id, int to_id) {
    for(auto i = 0; i < moves.size(); i++) {
        if(moves[i].from_id == from_id && moves[i].to_id == to_id)
            return i;
    }

    //если не нашли
    job_move new_move = {
        .from_id = from_id,
        .to_id = to_id,
        .jobs = 0
    };
    moves.push_back(new_move);
    return moves.size() - 1;
}

void get_min_and_max(my_msg msg, int& min_index, int& max_index) {
    min_index = 0;
    max_index = 0;
    for (int i = 0; i < msg.n; i++)
    {
        if(msg.jobs[i] < msg.jobs[min_index]) {
            min_index = i;
        }

        if(msg.jobs[i] > msg.jobs[max_index]) {
            max_index = i;
        }
    }
}

std::vector<job_move> generate_moves(my_msg &msg) {
    std::vector<job_move> moves;
    int min_index;
    int max_index;
    get_min_and_max(msg, min_index, max_index);
    while (abs(msg.jobs[min_index] - msg.jobs[max_index]) > 1)
    {
        int from_id = msg.ids[max_index];
        int to_id = msg.ids[min_index];
        int move_index = get_or_put(moves, from_id, to_id);
        moves[move_index].jobs += 1;
        msg.jobs[max_index] -= 1;
        msg.jobs[min_index] += 1;
        get_min_and_max(msg, min_index, max_index);
    }

    return moves;   
}
