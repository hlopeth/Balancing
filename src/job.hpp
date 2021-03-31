#include <random>
#include <list>
#define JOB_RAND_MAX 2048

struct t_job{
    float room_prise; //стоимость номера
    int days; //время пребывания в днях
    bool has_pansion; //наличие пансиона
    float dist_to_coast; //расстояние до моря
};

bool operator==(const t_job& a, const t_job& b) {
    return 
        a.room_prise == b.room_prise &&
        a.days == b.days &&
        a.has_pansion == b.has_pansion &&
        a.dist_to_coast == b.dist_to_coast;
}

bool operator!=(const t_job& a, const t_job& b) {
    return !(a==b);
}

const t_job NULL_JOB = {};


int random_val() {
    static std::random_device r;
    static std::default_random_engine el(r());
    static std::uniform_int_distribution<int> distr(0, JOB_RAND_MAX);
    return distr(el);
}

bool test_rate(float rate) {
    return float(random_val()) / JOB_RAND_MAX < rate;
}

t_job generate_job(float rate) {
    bool new_job = test_rate(rate);
    if(new_job) {
        t_job job = {
            .room_prise = random_val(),
            .days = random_val(),
            .has_pansion = test_rate(0.5),
            .dist_to_coast = random_val()
        };
        return job;
    } else {
        return NULL_JOB;
    }
}

float get_load(std::list<t_job> jobs) {
    return jobs.size();
}