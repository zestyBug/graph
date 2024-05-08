#ifndef __SOLVER_HPP
#define __SOLVER_HPP

#include <vector>
#include <unordered_map>
#include "span.hpp"

enum Scores : int {
    idle_hour = 10,
    unavailable_hour = -15,
    reserved_hour = -50,
    group_multiplier = -10
};

constexpr int hour_offset = 2;

struct EnchantProfessor;
struct EnchantLession;
struct EnchantGroup;

struct EnchantLession {
    // 2, 3 or 4
    char hour;
    // 1 ~ 8
    char semister;
    // maps the lession conflict with priority
    std::unordered_map<const EnchantLession*,int> conflicts;
    EnchantProfessor *assigned_professor;
    EnchantLession() : assigned_professor{nullptr} {}
};
struct ConflictBuffer {
    unsigned char count;
    const EnchantLession *lession;
    ConflictBuffer():count{0},lession{nullptr} {}
};
struct EnchantProfessor {
    // free times 0 score
    // served time -50 + -100 per each serve!
    //[i*2]:even
    //[i*2+1]:odd
    int hours_score[5*5*2];
    std::vector<EnchantLession*> assigned_lessions;
    EnchantProfessor() = default;
};
struct EnchantProfessorDynamic {
    int max_dynamic_hours = 0;
    bool dynamic_hours[5*5*2];
    EnchantProfessorDynamic() = default;
};
struct EnchantGroup {
    int score = 0;
    std::vector<const EnchantLession*> members;
    EnchantGroup() = default;
};

void rsort(span<ConflictBuffer>  array);

struct ProfessorBuffer {
    EnchantLession *hours_score[5*5*2];
    ProfessorBuffer() {
        memset(hours_score,0,sizeof(hours_score));
    }
};

struct TimeTable {
    std::vector<EnchantLession*> hours[5*5*2];
    TimeTable() = default;
};

int Score(
    const span<const EnchantLession>,
    const span<const EnchantProfessor>,
    const span<const EnchantProfessorDynamic>,
    const span<const EnchantGroup>
);

void solve_function(float& progress);

#endif
