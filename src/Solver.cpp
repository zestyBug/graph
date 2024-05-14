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



TimeTable::TimeTable() :
    lessions{ Lession::registery.pool_size() },
    conflicts{ Lession::registery.pool_size() },
    professors_buffer{ Professor::registery.pool_size() },
    freetime_buffer{ Professor::registery.pool_size() },
    professors_dynamic{ Professor::registery.pool_size() },
    step{ 0.49f / (float)(Lession::registery.pool_size()) }
{
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
            // Note All Conflict Types must be symmetric and duplex
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
        ProfessorBuffer& buffer = professors_buffer[index];
        EnchantProfessorDynamic& pointer_dynamics = professors_dynamic[index];
        professor_id_proxy.emplace(Professor::registery.entity(entt), &buffer);
        buffer.name = origin.name.c_str();
        for (size_t i = 0; i < 25; i++)
        {
            if (origin.hours[i]) {
                EnchantLession& freetime = freetime_buffer[index];
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
    for (size_t index=0;index < freetime_buffer.size();index++)
    {
        EnchantLession& freetime = freetime_buffer[index];
        freetime.hour = 0;
        freetime.assigned_professor = &professors_buffer[index];
        freetime.name = "Freetime";
        freetime.semister = -1;
        professors_buffer[index].hours[50] = &freetime;
    }

    sthread = std::thread(&TimeTable::solve_function, this);
}

void TimeTable::solve_function()
{
    try {
        // rand seed context is thread specified.
        auto ctime = 1715684329;//time(NULL);
        srand(ctime);
        printf("srand(%d);\n", ctime);
        // sort by most conflicts.
        this->rsort(this->conflicts);
        progress = 0.01f;
        // call each lession by most conflicts.
        for (const ConflictBuffer& lession : conflicts) {
            this->recursive_solve(*lession.lession);
            progress += this->step;
        }
        progress = 0.50f;
        this->verify_result();
        this->update_conflicts();
        progress = 0.51f;
        for (const ConflictBuffer& lession :conflicts)
        {
            this->iterative_update(*lession.lession);
            this->update_conflicts();
            progress += this->step;
        }
    } catch(const std::exception& e) {
        terminal.append("std::exception: ");
        terminal.append(e.what());
        terminal.append("\n");
    }
    progress=1.0f;
}

void TimeTable::verify_result() {
    for (int i = 0; i < 50; i++) {
        auto& hour = this->hours[i];
        for (const auto& value : hour) {
            const EnchantLession& this_lession = *value.lession;
            if (this_lession.hour < 1) {
                throw std::exception("verify_result failed");
            }
            const ProfessorBuffer& pf = *this_lession.assigned_professor;
            const EnchantLession& ls = *pf.hours[i];
            if (&ls != &this_lession) {
                throw std::exception("verify_result failed");
            }
        }
    }
    for (const auto& value : this->professors_buffer) {
        for (int i = 0; i < 50;i++) {
            const EnchantLession& this_lession = *value.hours[i];
            if (&this_lession!=nullptr  && this_lession.hour > 0) {
                if (this->hours[i].count(Result{ &this_lession }) < 1) {
                    throw std::exception("verify_result failed");
                }
            }
        }
    }
}

void TimeTable::apply_result(ScoreStack &res) {
    while (!res.choice_queue.empty())
    {
        const Choice& value = res.choice_queue.front();
        const int index = value.position;
        const EnchantLession* new_value = value.lession;
        if (index < 0 || index > 49)
            throw std::runtime_error("undefined hour");
        span<EnchantLession*> prof_hours{ new_value->assigned_professor->hours,51 };
        // update new positions
        const EnchantLession* old_value = prof_hours[index];

        // TODO: may be buggy!
        if (new_value->hour < 1) {
            this->hours[index].erase(Result{ old_value,0 });
            prof_hours[index] = prof_hours[50];
        }
        else {
            this->hours[index].erase(Result{old_value,0});
            prof_hours[index] = (EnchantLession*)new_value;
            this->hours[index].emplace( Result { new_value, res.total });
        }
        printf("update: { professor: %s, lession: %s, new pos: %d, old value: %s}\n", 
            new_value->assigned_professor->name, new_value->name, index, old_value->name );
        res.choice_queue.pop();
    }
    printf("\n");
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

// TODO: it must iterrate over lessions
// not over days week because result of update_conflicts is bigger than other_conflicts (due to some bug)
// and it causes to move same lession to diffrent places over and over again
// so even update_conflicts must not be called after every iterative_update
bool TimeTable::iterative_update(const EnchantLession& this_lession) {
    for (int i = 0; i < 50; i += 2) {
        auto& day_even = this->hours[i];
        auto& day_odd = this->hours[i + 1];
        for (const auto& value : day_even) {
            const EnchantLession* c = value.lession;
            if (&this_lession != c)
                continue;
            ProfessorBuffer buffer{*c->assigned_professor};
            if (day_odd.count(value)) {
                buffer.hours[i] = c->assigned_professor->hours[50];
                buffer.hours[i+1] = c->assigned_professor->hours[50];
                auto result = recursive_conflicts2(*c, buffer, SERACH_DEPTH);
                if (result.total >= value.score)
                    continue;
                printf("a better solution was found for %s (previous:%d, new: %d)\n", c->name, value.score, result.total);
                this->apply_result(result);
                goto exit;
            }
            else {
                buffer.hours[i] = c->assigned_professor->hours[50];
                auto result = recursive_conflicts1(*c, buffer, SERACH_DEPTH);
                if (result.total >= value.score)
                    continue;
                printf("a better solution was found for %s (previous:%d, new: %d)\n", c->name, value.score, result.total);
                this->apply_result(result);
                goto exit;
            }
        }
        for (const auto& value : day_odd) {
            if (!day_even.count(value)) {
                const EnchantLession* c = value.lession;
                if (&this_lession != c)
                    continue;
                ProfessorBuffer buffer{ *c->assigned_professor };
                buffer.hours[i+1] = c->assigned_professor->hours[50];
                auto result = recursive_conflicts1(*c, buffer, SERACH_DEPTH);
                if (result.total >= value.score)
                    continue;
                printf("a better solution was found for %s (previous:%d, new: %d)\n", c->name, value.score, result.total);
                this->apply_result(result);
                goto exit;
            }
        }
    }
    return false;
exit:;
    return true;
}


void TimeTable::print_result() {
    try {
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
    catch (const std::exception& e) {
        terminal.append("Error: ");
        terminal.append(e.what());
        terminal.append("\n");
    }
}

void TimeTable::rsort(span<ConflictBuffer> array) noexcept {
    for (size_t i = 0; i < array.size(); i++)
        for (size_t j = i+1; j < array.size(); j++)
            if(array[j].count > array[i].count)
                std::swap(array[j],array[i]);
}


Score TimeTable::other_conflicts(int i,const EnchantLession & this_value, Score give_up_score, bool finalize){
    //
    int lession_count = 0;
    Score total = 0;
    if (i > 49 || i < 0) {
        throw std::exception("invalide index");
    }
    for(const auto &value:this->hours[i]) {
        const EnchantLession * other_lession = value.lession;
        // it means other_conflicts is called by update_conflicts
        if (other_lession == &this_value)
            continue;
        // it means other_conflicts is called before swapping.
        // TODO: may be called by update_conflicts, 
        // in this case score must be Scores::conflict_unavailable_hour
        if (other_lession->assigned_professor == this_value.assigned_professor)
        {
            if (finalize)
                total = Scores::conflict_unavailable_hour;
            else
                continue;
        }
        // this line should be here
        lession_count++;
        if(other_lession->hour < 1)
            throw std::runtime_error("unexpected freetime in timetable");
        const auto res = this_value.conflicts.find(other_lession);
        const auto end = this_value.conflicts.end();
        // 1 apply group and semister conflicts, if there any
        if(res != end)
            total += res->second;
        // 2 apply same semister conflicts
        if (other_lession->semister == this_value.semister && this_value.semister > 0 && this_value.semister < 9)
            total += Scores::conflict_same_semister;
        // 3 odds
        else if (other_lession->semister > 8 || this_value.semister > 8)
            total += Scores::conflict_all_semister;
        //if (other_lession->semister < 1)
        else total += Scores::conflict_no_semister;

        if (total > give_up_score) {
            return total;
        }
    }
    // 4 less than 3 lession per hour to avoid rush hours
    // note: since lession is about to be added we must calculate what happen after it.
    if(lession_count > PRESENTATION_LIMIT -1)
        total += Scores::conflict_rush_hour;
    if(this_value.semister < 8 && this_value.semister > 0)
        if(Setting::forbiden[this_value.semister-1][i/2])
            total += Scores::conflict_banned_hour;
    return total;
}

void TimeTable::update_conflicts()
{
    for (const auto i : range<0, 50, 2>()) {
        for (auto& this_value : this->hours[i]) {
            const EnchantLession& this_lession = *this_value.lession;
            this_value.score = other_conflicts(i, this_lession, Scores::conflict_max);
        }
    }
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
            const Score other_conflicts_score = this->other_conflicts(i, lession, best_result.total);
            if (other_conflicts_score > best_result.total)
                continue;
            // selected hour
            const EnchantLession * hour = professor.hours[i];
            if(hour == nullptr)
            {
                result_temp.total = Scores::conflict_unavailable_hour;
                result_temp.choice_queue.emplace(&lession,i);
            }
            else if(hour->hour < 1)
            {
                result_temp.total = Scores::conflict_free_hour;
                result_temp.choice_queue.emplace(&lession,i);
            }
            else
            {
                ProfessorBuffer professor_temp(professor);
                if (hour->assigned_professor != lession.assigned_professor) {
                    throw std::runtime_error("something went wrong (3)");
                }
                // if this hour and next hour have same lession (non free)
                // note: there is no such situation where 'i' is an odd (eg: 1)
                // and previous hour have same lession but we only move 2nd lession
                // because it starts from 0 (an even nomber)
                if(!(i & 1) && professor.hours[i] == professor.hours[i+1]){
                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i+1] = professor.hours[50];
                    result_temp = this->recursive_conflicts2(*hour, professor_temp, level - 1);
                    // add a freetime in second lession we replaced
                    // in cases where nothing replace it and previous lession is still here
                    std::queue temp = std::move(result_temp.choice_queue);
                    result_temp.choice_queue.emplace(professor.hours[50], i+1);
                    while (!temp.empty())
                    {
                        result_temp.choice_queue.emplace(temp.front());
                        temp.pop();
                    }
                }else{
                    professor_temp.hours[i] = nullptr;
                    result_temp = this->recursive_conflicts1(*hour, professor_temp, level - 1);
                }
                //if(result_temp.total > Scores::conflict_max)
                //        continue;
                result_temp.choice_queue.emplace(&lession,i);
                result_temp.total += Scores::conflict_lession_swap;
            }
            result_temp.total += other_conflicts_score;
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
            const Score other_conflicts_score = 
                this->other_conflicts(i, lession, best_result.total) +
                this->other_conflicts(i+1, lession, best_result.total);
            if (other_conflicts_score > best_result.total)
                continue;

            if (hour1 == nullptr && hour2 == nullptr) {
                result_temp.total = Scores::conflict_unavailable_hour * 2;
                result_temp.choice_queue.emplace(&lession, i);
                result_temp.choice_queue.emplace(&lession, i + 1);
            }
            else if (hour1 && hour1->hour < 1 && hour2 && hour2->hour < 1)
            {
                result_temp.total = Scores::conflict_free_hour * 2;
                result_temp.choice_queue.emplace(&lession, i);
                result_temp.choice_queue.emplace(&lession, i + 1);
            }
            else
            {
                if (hour1 && hour1->hour > 0 && hour1->assigned_professor != lession.assigned_professor)
                    throw std::runtime_error("something went wrong (1)");
                if (hour2 && hour2->hour > 0 && hour2->assigned_professor != lession.assigned_professor)
                    throw std::runtime_error("something went wrong (2)");
                if (hour1 == hour2)
                {
                    ProfessorBuffer professor_temp(professor);
                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i + 1] = nullptr;
                    result_temp = this->recursive_conflicts2(*hour1, professor_temp, level - 1);
                    //if (result_temp.total > Scores::conflict_max)
                    //    continue;
                    result_temp.total += Scores::conflict_lession_swap * 2;
                    result_temp.choice_queue.emplace(&lession, i);
                    result_temp.choice_queue.emplace(&lession, i + 1);
                }
                else if (hour1 && hour1->hour > 0 && hour2 && hour2->hour > 0)
                {
                    // hold updated hours according to our first conflict choices
                    ProfessorBuffer professor_temp(professor);
                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i + 1] = nullptr;
                    result_temp = this->recursive_conflicts1(*hour1, professor_temp, level - 1);
                    //if (result_temp.total > Scores::conflict_max)
                    //    continue;

                    {
                         std::queue<Choice> temp = result_temp.choice_queue;
                         while (!temp.empty()) {
                             const Choice& value = temp.front();
                             professor_temp.hours[value.position] = (EnchantLession*)value.lession;
                             temp.pop();
                         }
                    }

                    ScoreStack result_temp2 = this->recursive_conflicts1(*hour2, professor_temp, level - 1);
                    //if (result_temp2.total > Scores::conflict_max)
                    //    continue;

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
                        if (hour1->hour < 1) {
                            result_temp.total = Scores::conflict_unavailable_hour + Scores::conflict_free_hour;
                            goto end;
                        }
                    }else{
                        if (hour1->hour < 1)
                            std::swap(hour1, hour2);
                    }
                    if (hour1 == nullptr || hour1->hour < 1)
                        throw std::runtime_error("unhandlable hour!");
                    if (hour2 != nullptr && hour2->hour > 0)
                        throw std::runtime_error("unhandlable hour!");

                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i + 1] = nullptr;
                    result_temp = this->recursive_conflicts1(*hour1, professor_temp, level - 1);
                    //if (result_temp.total > Scores::conflict_max)
                    //    continue;
                    result_temp.total += Scores::conflict_lession_swap;
                    result_temp.total += hour2 == nullptr ? Scores::conflict_unavailable_hour : Scores::conflict_free_hour;
                end:
                    result_temp.choice_queue.emplace(&lession, i);
                    result_temp.choice_queue.emplace(&lession, i + 1);
                }
            }
            result_temp.total += other_conflicts_score;
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
    ProfessorBuffer professor_new(professor);
    ScoreStack best_result;
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
