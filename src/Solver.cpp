#include "uni.hpp"
#include "ui_components.hpp"
#include <array>
#include "span.hpp"
#include "range.hpp"
#include "Solver.hpp"
#include <stack>
#include <stdlib.h>
#include <time.h>
#include <fstream>

Array::registry<Group> Group::registery;
Array::registry<Professor> Professor::registery;
Array::registry<Lession> Lession::registery;
std::map<Array::entity,Array::entity> Pair::map;
std::array<bool[25],8> Setting::forbiden;
std::string terminal;

static EnchantLession freetime;


TimeTable::TimeTable() :
    lessions{ Lession::registery.pool_size() },
    conflicts{ Lession::registery.pool_size() },
    professors_buffer{ Professor::registery.pool_size() },
    professors_dynamic{ Professor::registery.pool_size() }
{
    freetime.name = "freetime";
    const auto lession_count = Lession::registery.pool_size();
    const auto professor_count = Professor::registery.pool_size();
    if (lession_count == 0 || professor_count == 0)
        throw std::range_error("unexpected zero lession or professor count");

    std::map<Array::entity, EnchantLession*> lession_id_proxy;
    std::map<Array::entity, ProfessorBuffer*> professor_id_proxy;

    size_t index;
    // initialize lessions and conflicts
    index = 0;
    auto lview = Lession::registery.view();
    for (auto entt : lview) {
        const auto& origin = lview.get(entt);
        auto& copy = lessions[index];
        auto& conflict = conflicts[index];
        lession_id_proxy.emplace(Lession::registery.entity(entt), &copy);
        copy.name = origin.name.c_str();
        copy.hour = origin.hours + hour_offset;
        copy.semister = origin.semister;
        conflict.lession = &copy;
        conflict.count = origin.weight;
        index++;
    }
    // initialize lessions require & update conflicts
    for (auto entt : lview) {
        const auto& value = lview.get(entt);
        if (Array::null != value.require)
        {
            EnchantLession* require = lession_id_proxy.at(Lession::registery.entity(entt));
            EnchantLession* required = lession_id_proxy.at(Lession::registery.entity(value.require));
            // both gets conflict count
#ifdef _MSC_VER
            conflicts[require - lessions.data()].count++;
            conflicts[required - lessions.data()].count++;
#else
            conflicts[require - lessions].count++;
            conflicts[required - lessions].count++;
#endif
            require->set_conflict(required, Scores::conflict_requires);
            required->set_conflict(require, Scores::conflict_requiredby);
        }
    }
    // initialize groups & update conflicts
    index = 0;
    auto gview = Group::registery.view();
    for (auto entt : gview)
    {
        const Group& origin = gview.get(entt);
        for (auto member : origin.members)
        {
            EnchantLession* lession_pointer = lession_id_proxy.at(Lession::registery.entity(member));
            // each lession get one conflict count per each group it joint
#ifdef _MSC_VER
            conflicts[lession_pointer - lessions.data()].count++;
#else
            conflicts[lession_pointer - lessions].count++;
#endif
            for (auto other : origin.members)
            {
                const EnchantLession* other_pointer = lession_id_proxy.at(Lession::registery.entity(other));
                if (other_pointer != lession_pointer)
                {
                    lession_pointer->set_conflict(other_pointer, (origin.priority + 1) * Scores::conflict_group_multiplier);
                }
            }
        }
    }
    // initialize professors
    index = 0;
    auto pview = Professor::registery.view();
    for (auto entt : pview) {
        const Professor& origin = pview.get(entt);
        auto& buffer = professors_buffer[index];
        auto& pointer_dynamics = professors_dynamic[index];
        professor_id_proxy.emplace(Professor::registery.entity(entt), &buffer);
        buffer.name = origin.name.c_str();
        for (size_t i = 0; i < 25; i++)
        {
            if (origin.hours[i]) {
                buffer.hours[i * 2] = &freetime;
                buffer.hours[i * 2 + 1] = &freetime;
            }
            else {
                buffer.hours[i * 2] = nullptr;
                buffer.hours[i * 2 + 1] = nullptr;
            }
        }
        for (size_t i = 0; i < 25; i++)
        {
            if (origin.uncertain_hours[i]) {
                pointer_dynamics.dynamic_hours[i * 2] = 1;
                pointer_dynamics.dynamic_hours[i * 2 + 1] = 1;
            }
            else {
                pointer_dynamics.dynamic_hours[i * 2] = 0;
                pointer_dynamics.dynamic_hours[i * 2 + 1] = 0;
            }
        }
        // optimize it away.
        pointer_dynamics.max_dynamic_hours = origin.max_uncertain_hours;
        index++;
    }
    // pair lessions with professors
    index = 0;
    for (const auto& pair : Pair::map)
    {
        EnchantLession* less = lession_id_proxy.at(Lession::registery.entity(pair.first));
        ProfessorBuffer* prof = professor_id_proxy.at(Professor::registery.entity(pair.second));
        less->assigned_professor = prof;
    }
}

void TimeTable::solve_function(float& progress)
{
    try {
        // rand seed context is thread specified.
        srand(time(NULL));
        // sort by most conflicts.
        this->rsort(this->conflicts);
        progress = 0.01f;
        // call each lession by most conflicts.
        const float steps = 0.45f / (float)conflicts.size();
        for (const ConflictBuffer& lession : conflicts) {
            this->recursive_solve(*lession.lession);
            progress += steps;
        }
        this->update_conflicts();
    } catch(const std::exception& e) {
        terminal.append("std::exception: ");
        terminal.append(e.what());
        terminal.append("\n");
    }
    progress=1.0f;
}


void TimeTable::apply_result(ScoreStack &res) {
    while (!res.choice_queue.empty())
    {
        const Choice& value = res.choice_queue.front();
        const int index = value.position;
        const EnchantLession* new_value = value.lession;
        if (index < 0 || index > 49)
            throw std::runtime_error("undefined hour");
        span<EnchantLession*> prof_hours { value.lession->assigned_professor->hours,50 };

        // update new positions
        const EnchantLession* old_value = prof_hours[index];

        // TODO: may be buggy!
        this->hours[index].erase(Result{old_value,0});

        /*auto begin = this->hours[index].begin();
        auto end = this->hours[index].end();
        for (;begin != end;begin++) 
            if ((*begin).lession == old_value) {
                this->hours[index].erase(begin, begin);
                break;
            }*/


        // if () printf("delete: { lession: %s, from: %d}\n", old_value->name, index);

        prof_hours[index] = (EnchantLession*)new_value;
        this->hours[index].emplace( Result { new_value, res.total });
        res.choice_queue.pop();

        printf("update: { professor: %s, lession: %s, new pos: %d, old value: %s}\n", 
            new_value->assigned_professor->name, new_value->name, index, old_value->name );
    }
}

void TimeTable::recursive_solve(const EnchantLession& lession)
{
    ScoreStack(TimeTable:: * conflict_function)(const EnchantLession & lession, const ProfessorBuffer & professor, const unsigned char level);
    if (lession.hour == 2)
        conflict_function = &TimeTable::recursive_conflicts2;
    else if (lession.hour == 3)
        conflict_function = &TimeTable::recursive_conflicts3;
    else if (lession.hour == 4)
        conflict_function = &TimeTable::recursive_conflicts4;
    else
        throw std::runtime_error("unexpected lession::hour");

    if (!lession.assigned_professor)
        throw std::runtime_error("unexpected empty lession::professor value");

    ScoreStack res;
    res = (this->*conflict_function)(
        lession,
        *lession.assigned_professor,
        SERACH_DEPTH
    );
    if (res.total > Scores::conflict_max) {
        terminal.append("Error: ");
        terminal.append(lession.name);
        terminal.append("\n");
        throw std::runtime_error("no solution was found");
    }
    /*if(((const Choice&)res.choice_queue.front()).lession != &lession)
        throw std::runtime_error("unexpected solution");*/
    this->apply_result(res);
}


void TimeTable::print_result() {
    const char * STR_TEMPLATE = "{%s days: %s, lession: %s, professor: %s}\n";
    constexpr int str_len = 1023;
    char str[str_len+1];
    std::ofstream file{ "result.txt" };
    for (int i = 0; i < 50; i += 2) {
        auto& day_even = this->hours[i];
        auto& day_odd = this->hours[i + 1];
        for (const auto &value : day_even) {
            const EnchantLession* c = value.lession;
            const char* day_type;
            if (day_odd.count(value))
                day_type = "both";
            else
                day_type = "even";
            snprintf(str, str_len, STR_TEMPLATE, day_type, lable_name[i / 2], c->name, c->assigned_professor->name);
            file << str;
        }
        for (const auto& value : day_odd) {
            const EnchantLession* c = value.lession;
            if (!day_even.count(value)) {
                snprintf(str, str_len, STR_TEMPLATE, " odd", lable_name[i / 2], c->name, c->assigned_professor->name);
                file << str;
            }
        }
    }
}

void TimeTable::rsort(span<ConflictBuffer> array) noexcept {
    for (size_t i = 0; i < array.size(); i++)
        for (size_t j = i+1; j < array.size(); j++)
            if(array[j].count > array[i].count)
                std::swap(array[j],array[i]);
}


Score TimeTable::other_conflicts(int i,const EnchantLession &lession){
    //
    int lession_count = 0;
    Score total = 0;
    if (i > 49 || i < 0) {
        throw std::exception("invalide index");
    }
    for(const auto &value:this->hours[i]) {
        const EnchantLession * other_lession = value.lession;
        if(other_lession->assigned_professor == lession.assigned_professor)
            continue;
        if(other_lession == &freetime)
            throw std::runtime_error("unexpected freetime in timetable");
        const auto res = lession.conflicts.find(other_lession);
        const auto end = lession.conflicts.end();
        lession_count++;
        // 1 apply group and semister conflicts, if there any
        if(res != end)
            total += res->second;
        // 2 apply same semister conflicts
        if (other_lession->semister == lession.semister && lession.semister > 0 && lession.semister < 9)
            total += Scores::conflict_same_semister;
        // 3 odds
        else if (other_lession->semister > 8 || lession.semister > 8)
            total += Scores::conflict_all_semister;
        //if (other_lession->semister < 1)
        else
            total += Scores::conflict_no_semister;
    }
    // 4 less than 3 lession per hour to avoid rush hours
    if(lession_count > PRESENTATION_LIMIT)
        total += Scores::conflict_rush_hour;
    if(lession.semister < 8 && lession.semister > 0)
        if(Setting::forbiden[lession.semister-1][i/2])
            total += Scores::conflict_banned_hour;
    return total;
}

ScoreStack TimeTable::recursive_conflicts1 (
    const EnchantLession &lession,
    const ProfessorBuffer &professor,
    const unsigned char level
){
    ScoreStack best_result;
    if(level!=0)
    {
        for (const auto i : range<0, 50, 1>())
        {
            ScoreStack result_temp;
            // selected hour
            const EnchantLession * hour = professor.hours[i];
            if(hour == nullptr)
            {
                result_temp.total = Scores::conflict_unavailable_hour;
                result_temp.choice_queue.emplace(&lession,i);
            }
            else if(hour == &freetime)
            {
                result_temp.total = Scores::conflict_free_hour;
                result_temp.choice_queue.emplace(&lession,i);
            }
            else
            {
                ProfessorBuffer professor_temp(professor);
                if(hour->assigned_professor != lession.assigned_professor)
                    throw std::runtime_error("something went wrong");
                // if this hour and next hour have same lession (non free)
                if(!(i & 1) && professor.hours[i] == professor.hours[i+1]){
                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i+1] = nullptr;
                    result_temp = this->recursive_conflicts2(*hour, professor_temp, level - 1);
                }else{
                    professor_temp.hours[i] = nullptr;
                    result_temp = this->recursive_conflicts1(*hour, professor_temp, level - 1);
                }
                if(result_temp.total > Scores::conflict_max)
                        continue;
                result_temp.choice_queue.emplace(&lession,i);
                result_temp.total += Scores::conflict_lession_swap;
            }
            result_temp.total += this->other_conflicts(i,lession);
            best_result.swap_if_better(result_temp);
        }
    }
    return best_result;
}

ScoreStack TimeTable::recursive_conflicts2(
    const EnchantLession& lession,
    const ProfessorBuffer& professor,
    const unsigned char level
) {
    ScoreStack best_result;
    if (level != 0) {
        for(const auto i:range<0,50,2>())
        {
            ScoreStack result_temp;
            const EnchantLession* hour1 = professor.hours[i];
            const EnchantLession* hour2 = professor.hours[i + 1];
            if (hour1 == nullptr && hour2 == nullptr) {
                result_temp.total = Scores::conflict_unavailable_hour * 2;
                result_temp.choice_queue.emplace(&lession, i);
                result_temp.choice_queue.emplace(&lession, i + 1);
            }
            else if (hour1 == &freetime && hour2 == &freetime)
            {
                result_temp.total = Scores::conflict_free_hour * 2;
                result_temp.choice_queue.emplace(&lession, i);
                result_temp.choice_queue.emplace(&lession, i + 1);
            }
            else
            {
                if (hour1 && hour1 != &freetime && hour1->assigned_professor != lession.assigned_professor)
                    throw std::runtime_error("something went wrong");
                if (hour2 && hour2 != &freetime && hour2->assigned_professor != lession.assigned_professor)
                    throw std::runtime_error("something went wrong");
                if (hour1 == hour2)
                {
                    ProfessorBuffer professor_temp(professor);
                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i + 1] = nullptr;
                    result_temp = this->recursive_conflicts2(*hour1, professor_temp, level - 1);
                    if (result_temp.total > Scores::conflict_max)
                        continue;
                    result_temp.total += Scores::conflict_lession_swap * 2;
                    result_temp.choice_queue.emplace(&lession, i);
                    result_temp.choice_queue.emplace(&lession, i + 1);
                }
                else if (hour1 && hour1 != &freetime && hour2 && hour2 != &freetime)
                {
                    // hold updated hours according to our first conflict choices
                    ProfessorBuffer professor_temp(professor);
                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i + 1] = nullptr;
                    result_temp = this->recursive_conflicts1(*hour1, professor_temp, level - 1);
                    if (result_temp.total > Scores::conflict_max)
                        continue;

                    {
                         std::queue<Choice> temp = result_temp.choice_queue;
                         while (temp.empty()) {
                             const Choice& value = temp.front();
                             professor_temp.hours[value.position] = (EnchantLession*)value.lession;
                             temp.pop();
                         }
                    }

                    ScoreStack result_temp2 = this->recursive_conflicts1(*hour2, professor_temp, level - 1);
                    if (result_temp2.total > Scores::conflict_max)
                        continue;

                    result_temp.choice_queue.emplace(&lession, i);

                    while (!result_temp2.choice_queue.empty()) {
                        result_temp.choice_queue.emplace(result_temp2.choice_queue.front());
                        result_temp2.choice_queue.pop();
                    }

                    result_temp.choice_queue.emplace(&lession, i + 1);
                    result_temp.total += result_temp2.total;
                    result_temp.total += Scores::conflict_lession_swap * 2;
                }
                else
                {
                    ProfessorBuffer professor_temp(professor);

                    if (hour1 == nullptr || hour2 == nullptr) {
                        if (hour1 == nullptr)
                            std::swap(hour1, hour2);
                        if (hour1 == &freetime) {
                            result_temp.total = Scores::conflict_unavailable_hour + Scores::conflict_free_hour;
                            goto end;
                        }
                    }else{
                        if (hour1 == &freetime)
                            std::swap(hour1, hour2);
                    }
                    if (hour1 == nullptr || hour1 == &freetime)
                        throw std::runtime_error("unhandlable hour!");
                    if (hour2 != nullptr && hour2 != &freetime)
                        throw std::runtime_error("unhandlable hour!");

                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i + 1] = nullptr;
                    result_temp = this->recursive_conflicts1(*hour1, professor_temp, level - 1);
                    if (result_temp.total > Scores::conflict_max)
                        continue;
                    result_temp.total += Scores::conflict_lession_swap;
                    result_temp.total += hour2 == nullptr ? Scores::conflict_unavailable_hour : Scores::conflict_free_hour;
                end:
                    result_temp.choice_queue.emplace(&lession, i);
                    result_temp.choice_queue.emplace(&lession, i + 1);
                }
            }
            result_temp.total += this->other_conflicts(i, lession);
            result_temp.total += this->other_conflicts(i, lession);
            best_result.swap_if_better(result_temp);
        }
    }
    return best_result;
}

ScoreStack TimeTable::recursive_conflicts3(
    const EnchantLession& lession,
    const ProfessorBuffer& professor,
    const unsigned char level
) {
    ScoreStack best_result;
    ProfessorBuffer professor_new(professor);
    if (level != 0)
    {

        best_result = this->recursive_conflicts2(lession,professor,level);

        std::queue<Choice> temp{best_result.choice_queue};
        while (!temp.empty())
        {
            const Choice& value = temp.front();
            professor_new.hours[value.position] = (EnchantLession*)value.lession;
            temp.pop();
        }

        int count = 0;
        for (auto& hour : professor_new.hours) {
            if (hour == &lession) {
                hour = nullptr;
                count++;
            }
            //printf("\tlession: %s\n",hour ? hour->name :"-");
        }

        if (count > 2)
            throw std::runtime_error("lession was previusly allocated!");
        ScoreStack res2 = this->recursive_conflicts1(lession,professor_new,level);
        while (!res2.choice_queue.empty()) {
            best_result.choice_queue.emplace(res2.choice_queue.front());
            res2.choice_queue.pop();
        }
        best_result.total += res2.total;
    }
    return best_result;
}


ScoreStack TimeTable::recursive_conflicts4(
    const EnchantLession& lession,
    const ProfessorBuffer& professor,
    const unsigned char level
) {
    ScoreStack best_result;
    if (level != 0)
    {
        best_result = this->recursive_conflicts2(
            lession,
            professor,
            level
        );
        ProfessorBuffer professor_new(professor);

        std::queue<Choice> temp = best_result.choice_queue;
        while (temp.empty()) {
            const Choice& value = temp.front();
            professor_new.hours[value.position] = (EnchantLession*)value.lession;
            temp.pop();
        }

        int count = 0;
        for (auto& hour : professor_new.hours) {
            if (hour == &lession) {
                hour = nullptr;
                count++;
            }
        }
        if (count > 2)
            throw std::runtime_error("lession was previusly allocated!");
        ScoreStack res2 = this->recursive_conflicts2(
            lession,
            professor_new,
            level
        );
        while (!res2.choice_queue.empty()) {
            best_result.choice_queue.emplace(res2.choice_queue.front());
            res2.choice_queue.pop();
        }
    }
    return best_result;
}

void TimeTable::update_conflicts()
{
    for (const auto i : range<0, 50, 2>()) {
        Score total = 0;

        for (auto& this_value : this->hours[i]) {
            const EnchantLession& this_lession = *this_value.lession;
            int lession_count=0;
            for (const auto& other_value : this->hours[i])
            {
                lession_count++;
                const EnchantLession &other_lession = *other_value.lession;
                if (&other_lession == &this_lession)
                    continue;
                if (other_lession.assigned_professor == this_lession.assigned_professor) {
                    /*char err[512];
                    sprintf(err,"%s and %s on same hour with %s\n", 
                        other_lession.name, this_lession.name, this_lession.assigned_professor->name);
                    terminal.append(err);*/
                    total += Scores::conflict_unavailable_hour;
                }

                const auto res = this_lession.conflicts.find(&other_lession);
                // 1 apply group and semister conflicts, if there any
                if (res != this_lession.conflicts.end())
                    total += res->second;
                // 2 apply same semister conflicts
                if (other_lession.semister == this_lession.semister && 
                    this_lession.semister > 0 && this_lession.semister < 9)
                    total += Scores::conflict_same_semister;
                // 3 odds
                else if (other_lession.semister > 8 || this_lession.semister > 8)
                    total += Scores::conflict_all_semister;
                //if (other_lession->semister < 1)
                else
                    total += Scores::conflict_no_semister;

            }
            if (lession_count > PRESENTATION_LIMIT)
                total += Scores::conflict_rush_hour;
            if (this_lession.semister < 8 && this_lession.semister > 0)
                if (Setting::forbiden[this_lession.semister - 1][i / 2])
                    total += Scores::conflict_banned_hour;
            this_value.score = total;
        }
    }
}