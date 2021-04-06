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
#define MOVE_ID 2

struct t_procces {
    int id;
    int jobs;
    std::vector<int> out;
};

std::vector<t_procces> read_topology(std::string);
bool has(my_msg msg, int id);
bool has(move_t move, int id);
void add_to_msg(my_msg &bud, int id, int jobs);
move_t generate_moves(my_msg &msg);

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
        std::cout << "before moves:" << std::endl;
        for(int i = 0; i < msg.n; i++) {
            std::cout << "id: " << msg.ids[i] << " job " << msg.jobs[i] << std::endl;
        }
        std::cout << "moves: " << std::endl;
        auto moves = generate_moves(msg);
        for(int i = 0 ; i < moves.n; i++) {
            std::cout << "from " << moves.from_id[i] << " to " << moves.to_id[i] << " jobs " << moves.jobs[i] << std::endl;
        }
        // std::cout << "results:" << std::endl;
        // for(int i = 0; i < msg.n; i++) {
        //     std::cout << "id: " << msg.ids[i] << " job " << msg.jobs[i] << std::endl;
        // }

        moves.mtype = MOVE_ID;
        moves.m = 1;
        moves.visited_nodes[0] = pd.id;
        for(auto id: pd.out) {
            send_move(id_to_mqId[id], moves);
        }
        // отправляем самому себе что для общности кода у всех потоков
        moves.m = pd.out.size();
        for(int i = 0; i < pd.out.size(); i++) {
            moves.visited_nodes[i] = pd.out[i];
        }
        send_move(my_mq_id, moves); 
        
        
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


    //работа с move_t
    auto move = recv_move(my_mq_id, MOVE_ID);
    for(auto id: pd.out) {
        if(!has(move, id)) {
            move.visited_nodes[move.m] = id;
            move.m += 1;
            send_move(id_to_mqId[id], move);
        }
    }

    int now_jobs = pd.jobs;
    for(int i = 0; i < move.n; i++) {
        if(move.to_id[i] == pd.id) {
            now_jobs += move.jobs[i];
        }
        if(move.from_id[i] == pd.id) {
            now_jobs -= move.jobs[i];
        }
    }
    
    printf("my pid %i i new have %i jobs\n", pd.id, now_jobs);
    // std::cout << "my pid " << pd.id << " i now have " << now_jobs << " jobs" << std::endl;

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

bool has(move_t move, int id) {
    for(int i = 0; i < move.m; i++) {
        if(move.visited_nodes[i] == id) 
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


int get_or_put(move_t &moves, int from_id, int to_id) {
    for(auto i = 0; i < moves.n; i++) {
        if(moves.from_id[i] == from_id && moves.to_id[i] == to_id)
            return i;
    }

    //если не нашли
    moves.from_id[moves.n] = from_id;
    moves.to_id[moves.n] = to_id;
    moves.jobs[moves.n] = 0;
    moves.n += 1;
    return moves.n - 1;
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

move_t generate_moves(my_msg &msg) {
    move_t moves;
    moves.n = 0;

    int min_index;
    int max_index;
    get_min_and_max(msg, min_index, max_index);
    while (abs(msg.jobs[min_index] - msg.jobs[max_index]) > 1)
    {
        int from_id = msg.ids[max_index];
        int to_id = msg.ids[min_index];
        int move_index = get_or_put(moves, from_id, to_id);
        moves.jobs[move_index] += 1;
        msg.jobs[max_index] -= 1;
        msg.jobs[min_index] += 1;
        get_min_and_max(msg, min_index, max_index);
    }

    return moves;   
}
