#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <unistd.h>

#include "mqueue.hpp"
#include "json.hpp"
using json = nlohmann::json;

#define FINN_ID 1
#define MOVE_ID 2

struct t_procces {
    int id;
    int jobs;
    std::vector<int> in;
    std::vector<int> out;
    bool intiator;
};

std::vector<t_procces> read_topology(std::string);
bool has(move_t move, int id);
void add_to_msg(my_msg &bud, int id, int jobs);
move_t generate_moves(my_msg &msg);
bool has(int arr[], int n, int elem);
bool has(std::vector<int> vect, int elem);

//procecces data
t_procces pd;
int my_mq_id;
std::map<int, int> id_to_mqId;

int processes_count = 0;
std::vector<int> inc;
std::vector<int> jobs;
std::vector<int> n_inc;

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "no topology filename" << std::endl;
        return 0;
    }

    std::vector<t_procces> processes = read_topology(argv[1]);

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

    inc.push_back(pd.id);
    jobs.push_back(pd.jobs);

    //инициализация балансировки нагрузи в root
    if(pd.intiator) {
        for(auto it: pd.out) {
            finn_t finn;
            finn.mtype = FINN_ID;
            finn.inc[0] = pd.id;
            finn.inc_size = 1;
            finn.n_inc_size = 0;

            send_finn(id_to_mqId[it], finn);
        }
    }

    while (inc.size() < processes_count && n_inc.size() < processes_count)
    {
        finn_t msg = recv_finn(my_mq_id, FINN_ID);

        // std::cout << "my id " << pd.id << " i got msg where inc_size = " << msg.inc_size << " and n_inc_size = " << msg.n_inc_size << std::endl;

        //копируем себе все inc из сообщения
        for(int i = 0; i < msg.inc_size; i++) {
            if(!has(inc, msg.inc[i])) {
                inc.push_back(msg.inc[i]);
                jobs.push_back(msg.jobs[i]);
            }
        }

        //копируем себе все n_inc из сообщения
        for(int i = 0; i < msg.n_inc_size; i++) {
            if(!has(n_inc, msg.n_inc[i])) {
                n_inc.push_back(msg.n_inc[i]);
            }
        }

        //если у сообщения в inc нет нашего id, вставляем
        if(!has(msg.inc, msg.inc_size, pd.id)) {
            msg.inc[msg.inc_size] = pd.id;
            msg.jobs[msg.inc_size] = pd.jobs;
            msg.inc_size += 1;
        }

        //если получили сообщения от всех входящих вершин, вставляем id в n_inc
        bool has_all_inputs = true;
        for (auto in: pd.in)
        {
            if(!has(inc, in)) {
                has_all_inputs = false;
                break;
            }
        }
        if(has_all_inputs) {
            msg.n_inc[msg.n_inc_size] = pd.id;
            msg.n_inc_size += 1;
        }
        // std::cout << "my id " << pd.id << " i send msg where inc_size = " << msg.inc_size << " and n_inc_size = " << msg.n_inc_size << std::endl;
        
        for(auto out: pd.out) {
            send_finn(id_to_mqId[out], msg);
        }
    }  

    if(pd.intiator) {   

        std::cout << "moves: " << std::endl;
        //использую не имеющую отношение my_msg, потому что влом переписывать под новые типы
        my_msg msg;
        msg.n = inc.size();
        for (int i = 0; i < msg.n; i++)
        {
            msg.ids[i] = inc[i];
            msg.jobs[i] = jobs[i];
        }
        
        auto moves = generate_moves(msg);
        for(int i = 0 ; i < moves.n; i++) {
            std::cout << "from " << moves.from_id[i] << " to " << moves.to_id[i] << " jobs " << moves.jobs[i] << std::endl;
        }
    }

    msgctl(my_mq_id,IPC_RMID,NULL);
    return 0;
}

std::vector<t_procces> read_topology(std::string filename) {
    std::vector<t_procces> result;
    std::ifstream i(filename.c_str());
    json j;
    i >> j;
    int size = j["size"].get<int>();
    processes_count = size;
    int initiator = j["initiator"].get<int>();
    for(int i = 0; i < size; i++) {
        t_procces descr;
        descr.id = j["processes"][i]["id"].get<int>();
        descr.jobs = j["processes"][i]["jobs"].get<int>();
        descr.intiator = descr.id == initiator;
        auto out = j["processes"][i]["to"];
        for(auto element: out) {
            descr.out.push_back(element.get<int>());
        }
        auto in = j["processes"][i]["from"];
        for(auto element: in) {
            descr.in.push_back(element.get<int>());
        }
        result.push_back(descr);
    }
    return result;
}

bool has(int arr[], int n, int elem) {
    for(int i = 0; i < n; i++) {
        if(arr[i] == elem)
            return true;
    }
    return false;
}

bool has(std::vector<int> vect, int elem) {
    int n = vect.size();
    for(int i = 0; i < n; i++) {
        if(vect[i] == elem)
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
