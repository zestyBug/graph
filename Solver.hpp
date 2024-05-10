#ifndef __SOLVER_HPP
#define __SOLVER_HPP

#include <vector>
#include <unordered_set>
#include <stack>
#include <queue>
#include <unordered_map>
#include "span.hpp"
#include <cstring>
#include <stdexcept>


// remember:
// closer to 0 = better
// negative numbers = invalide
typedef unsigned int Score;

// conflict scores
// higher = more perirority
enum Scores : Score {
    conflict_free_hour = 0,
    conflict_group_multiplier = 2,
    conflict_requiredby = 10,
    conflict_requires = 10,
    conflict_group_low = 4,
    conflict_group_meduim = 7,
    conflict_group_high = 9,
    conflict_out_of_depth = 100000,
    conflict_unavailable_hour = 10000,
    conflict_lession_swap = 1,
    conflict_max = 9998,
};

constexpr int hour_offset = 2;

struct EnchantLession;
struct ProfessorBuffer;

struct EnchantLession {
    const char * name;
    // 2, 3 or 4
    char hour;
    // 1 ~ 8
    char semister;
    // maps the lession conflict with priority
    std::unordered_map<const EnchantLession*,Score> conflicts;
    ProfessorBuffer *assigned_professor;
    EnchantLession() noexcept  :name{ "" },hour{ 2 },semister { 1 },assigned_professor{ nullptr } {
    }
    // set or update conflict value
    void set_conflict(const EnchantLession* pointer,int score)
    {
        auto res = this->conflicts.find(pointer);
        if(res != this->conflicts.end())
        {
            int old_score = res->second;
            if(score > old_score)
                this->conflicts.at(pointer) = score;
        }
        else
        {
            auto result = this->conflicts.emplace(pointer,score);
            if(!result.second)
                throw std::runtime_error("unordered_map failed");
        }
    }
};
struct ConflictBuffer {
    unsigned char count;
    const EnchantLession *lession;
    ConflictBuffer() noexcept :count{0},lession{nullptr} {}
};
struct ProfessorBuffer {
    const char * name;
    EnchantLession *hours[5*5*2];
    ProfessorBuffer() noexcept :name{nullptr} {
        memset(this->hours,0,sizeof(this->hours));
    }
    ProfessorBuffer(const ProfessorBuffer &) = default;
};
struct EnchantProfessorDynamic {
    int max_dynamic_hours = 0;
    bool dynamic_hours[5*5*2];
    EnchantProfessorDynamic() = default;
};

void rsort(span<ConflictBuffer>  array);


struct TimeTable {
    std::unordered_set<const EnchantLession*> hours[5*5*2];
    TimeTable() = default;
};
struct TimeTableBuffer {
    Score hours[5*5*2];
    TimeTableBuffer() noexcept {
        memset(this->hours,0,sizeof(this->hours));
    }
};

void solve_function(float& progress);

constexpr unsigned int SERACH_DEPTH = 5;
struct Choice {
    const EnchantLession *lession;
    int position;
    Choice(const EnchantLession *l,int p):lession{l},position{p} {};
};
struct ScoreStack {
    Score total;
    std::queue<Choice> choice_queue;
    ScoreStack() noexcept :total{Scores::conflict_out_of_depth} {}
    ScoreStack(const ScoreStack&) = default;
    ScoreStack(ScoreStack &&) = default;
    ScoreStack& operator = (const ScoreStack&) = default;
    ScoreStack& operator = (ScoreStack&&) = default;
    void swap_if_better(const ScoreStack& stack_object){
        if(&stack_object != this){
            if(this->total > stack_object.total){
                this->total = stack_object.total;
                this->choice_queue = stack_object.choice_queue;
            }
        }
    }
};

// returns least conflict Score of given lession for 1 hour
ScoreStack recursive_conflicts1(
    const TimeTable& timetable,
    const EnchantLession& lession,
    const ProfessorBuffer& professor,
    const unsigned char level
);
// returns least conflict Score of given lession for 2 hour
ScoreStack recursive_conflicts2(
    const TimeTable& timetable,
    const EnchantLession& lession,
    const ProfessorBuffer& professor,
    const unsigned char level
);
// returns least conflict Score of given lession for 3 hour
ScoreStack recursive_conflicts3(
    const TimeTable& timetable,
    const EnchantLession& lession,
    const ProfessorBuffer& professor,
    const unsigned char level
);
// returns least conflict Score of given lession for 4 hour
ScoreStack recursive_conflicts4(
    const TimeTable& timetable,
    const EnchantLession& lession,
    const ProfessorBuffer& professor,
    const unsigned char level
);

typedef ScoreStack(*conflict_function_t)(
    const TimeTable& timetable,
    const EnchantLession& lession,
    const ProfessorBuffer& professor,
    const unsigned char level
);

void recursive_solve(const EnchantLession& lession, TimeTable& timetable);
void apply_result(ScoreStack& res, TimeTable& timetable);
void print_result(const TimeTable& timetable);

#endif
