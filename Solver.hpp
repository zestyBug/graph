#ifndef __SOLVER_HPP
#define __SOLVER_HPP

#include <vector>
#include <unordered_map>
#include "span.hpp"
#include <string.h>

enum Scores : int {
    idle_hour = 10,
    unavailable_hour = -15,
    reserved_hour = -50,
    group_multiplier = -10,
    conflict_requiredby = 10,
    conflict_requires = 10,
    conflict_group_low = 4,
    conflict_group_meduim = 7,
    conflict_group_high = 9,
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
    std::unordered_map<const EnchantLession*,int> conflicts;
    ProfessorBuffer *assigned_professor;
    EnchantLession() : assigned_professor{nullptr} {}
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
    ConflictBuffer():count{0},lession{nullptr} {}
};
struct ProfessorBuffer {
    const char * name;
    EnchantLession *hours_score[5*5*2];
    ProfessorBuffer() {
        memset(hours_score,0,sizeof(hours_score));
    }
};
struct EnchantProfessorDynamic {
    int max_dynamic_hours = 0;
    bool dynamic_hours[5*5*2];
    EnchantProfessorDynamic() = default;
};

void rsort(span<ConflictBuffer>  array);


struct TimeTable {
    std::vector<EnchantLession*> hours[5*5*2];
    TimeTable() = default;
};

void solve_function(float& progress);

#endif
