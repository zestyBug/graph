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
#include <stdlib.h>


// remember:
// closer to 0 = better
// negative numbers = invalide
typedef int Score;

constexpr int hour_offset = 2;
constexpr unsigned int SERACH_DEPTH = 5;
constexpr unsigned int PRESENTATION_LIMIT = 3;

// conflict scores
// higher = more perirority
enum Scores : Score {
    conflict_free_hour = 0,
    conflict_group_multiplier = 10,
    conflict_requiredby = 10,
    conflict_requires = 10,
    conflict_out_of_depth = 100000,
    conflict_no_solution = 1000000,
    conflict_unavailable_hour = 10000,
    conflict_lession_swap = 1,
    conflict_max = 9998,
    conflict_same_semister = 20,
    conflict_all_semister = 10,
    conflict_no_semister = 1,
    conflict_banned_hour = 100,
    conflict_rush_hour = 30,
};

struct EnchantLession;
struct ProfessorBuffer;

struct EnchantLession {
    const char * name;
    // 2, 3 or 4
    char hour;
    // 1 ~ 8: normal
    char semister;
    // maps the lession conflict with priority
    std::unordered_map<const EnchantLession*,Score> conflicts;
    // WARN: never ever touch it in recursive_conflictsX,
    // use professor parameter instead.
    // because may some hours has disabled for this given lession,
    // or for callculation sake.
    ProfessorBuffer *assigned_professor;
public:
    EnchantLession() noexcept  :name{ "" },hour{ 2 },semister { 1 },assigned_professor{ nullptr } {}
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

struct TimeTableBuffer {
    Score hours[5*5*2];
    TimeTableBuffer() noexcept {
        memset(this->hours,0,sizeof(this->hours));
    }
};
struct Choice {
    const EnchantLession *lession;
    int position;
    Choice(const EnchantLession *l,int p):lession{l},position{p} {};
};
struct ScoreStack {
    Score total;
    std::queue<Choice> choice_queue;
    ScoreStack() noexcept :total{Scores::conflict_no_solution} {}
    ScoreStack(const ScoreStack&) = default;
    ScoreStack(ScoreStack &&) = default;
    ScoreStack& operator = (const ScoreStack&) = default;
    ScoreStack& operator = (ScoreStack&&) = default;
    // swap for better result
    // note: uses std::move for coping choice stack
    // note: there is a 50% chance to swap a object with same score
    // in order to have randomized results
    void swap_if_better(ScoreStack& stack_object) noexcept {
        if(&stack_object != this){
            const bool chance = (rand() & 0b1111) < 4;
            if(
                this->total > stack_object.total ||
                (this->total == stack_object.total && chance) 
            ){
                this->total = stack_object.total;
                this->choice_queue = std::move(stack_object.choice_queue);
            }
        }
    }
};


/***********************************************************************************
* tecnicaly, recursive_conflictsX soppused to find a perfect hour with given times.
* it can move lessions ONLY with same Professor to a better place.
* to avoid complexity and time waste.
* but even you can imagin bad senarios.
* it is it weakness.
* 
* TOOO:
**  add a small penallty to using non-available times, by creating a variable
*   like 'freetime' varialbe,
**  more AI stuff
**  a final optimization over the result to reduce the conflicts.
**  better variable and function naming, instead of hour, less, prof, ...
**  cleaner code: more readable, better function definition
**  A give up parameter: if have found a way that scored better than current way already,
*   just give up and dont iterate over all possiblities and depthes.
**  A second iteration to update conflict value of lessions that was placed first
*   and moving them to reduce total conflict
**  saving and loading
**  more than single required lession
**  professor hour periority
*/




class TimeTable 
{
    struct Result {
        const EnchantLession* lession;
        mutable Score score;
    };
    struct cequal_to {
        _NODISCARD constexpr bool operator()(const Result& _Left, const Result& _Right) const {
            return _Left.lession== _Right.lession;
        }
    };
    struct chash {
        // hash functor primary template (handles enums, integrals, and pointers)
        static size_t _Do_hash(const Result& _Keyval) noexcept {
            return (intptr_t) _Keyval.lession;
        }
        _NODISCARD size_t operator()(const Result& _Keyval) const noexcept {
            return _Do_hash(_Keyval);
        }
    };
    // list of lessions
    // last one represent free time
    std::vector <EnchantLession> lessions;
    // count of conflicts with a refrence to the lession
    std::vector <ConflictBuffer> conflicts;
    // stores current professors time decisions
    std::vector <ProfessorBuffer> professors_buffer;
    // stores professors dynamic time information
    std::vector<EnchantProfessorDynamic> professors_dynamic;
    std::unordered_set<Result, chash, cequal_to> hours[5 * 5 * 2];
public:
    TimeTable();
    // this fucntion is so important, it converts ENTT array
    // to better structure. also countw number of conflicts.
    // then invoke the main proccess
    void solve_function(float& progress);
    // defined in uni.hpp
    // return true if we are done with it and should be deleted.
    bool panel();
protected:
    //A helper function 
    //calculte conflict score,
    //bitween given lession and array of other lessions,
    //if there was any.
    //note: ignores lessions with same professor.
    Score other_conflicts(int i, const EnchantLession& lession);
    void recursive_solve(const EnchantLession& lession);
    void apply_result(ScoreStack& res);
    void print_result();
    void update_conflicts();

    static void rsort(span<ConflictBuffer>  array) noexcept;

    // returns least conflict Score of given lession for 1 hour
    ScoreStack recursive_conflicts1(const EnchantLession& lession,const ProfessorBuffer& professor,const unsigned char level);
    // returns least conflict Score of given lession for 2 hour
    ScoreStack recursive_conflicts2(const EnchantLession& lession, const ProfessorBuffer& professor, const unsigned char level);
    // returns least conflict Score of given lession for 3 hour
    ScoreStack recursive_conflicts3(const EnchantLession& lession, const ProfessorBuffer& professor, const unsigned char level);
    // returns least conflict Score of given lession for 4 hour
    ScoreStack recursive_conflicts4(const EnchantLession& lession, const ProfessorBuffer& professor, const unsigned char level);
};


#endif