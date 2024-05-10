#include "uni.hpp"
#include "ui_components.hpp"
#include <array>
#include "span.hpp"
#include "range.hpp"
#include "Solver.hpp"
#include <stack>

Array::registry<Group> Group::registery;
Array::registry<Professor> Professor::registery;
Array::registry<Lession> Lession::registery;
std::map<Array::entity,Array::entity> Pair::map;
std::array<bool[25],8> Setting::forbiden;
std::string terminal;

static EnchantLession freetime;

void solve_function(float& progress)
{
    try {
        const auto lession_count = Lession::registery.pool_size();
        const auto professor_count = Professor::registery.pool_size();
        if (lession_count == 0 || professor_count == 0)
            throw std::range_error("unexpected zero lession or professor count");
#ifdef _MSC_VER
        // list of lessions
        // last one represent free time
        std::vector <EnchantLession> lessions{ lession_count };
        // count of conflicts with a refrence to the lession
        std::vector <ConflictBuffer> conflicts{ lession_count };
        // stores current professors time decisions
        std::vector <ProfessorBuffer> professors_buffer{ professor_count };
        // stores professors dynamic time information
        std::vector<EnchantProfessorDynamic> professors_dynamic{professor_count};
#else
    // list of lessions
    // last one represent free time
    EnchantLession lessions[lession_count];
    // count of conflicts with a refrence to the lession
    ConflictBuffer conflicts[lession_count];
    // stores current professors time decisions
    ProfessorBuffer professors_buffer[professor_count];
    // stores professors dynamic time information
    EnchantProfessorDynamic professors_dynamic[professor_count];
#endif //

    {
        std::map<Array::entity,EnchantLession*> lession_id_proxy;
        std::map<Array::entity,ProfessorBuffer*> professor_id_proxy;

        size_t index;
        // initialize lessions and conflicts
        progress=0.05;
        index=0;
        auto lview = Lession::registery.view();
        for(auto entt:lview) {
            const auto &origin = lview.get(entt);
            auto &copy = lessions[index];
            auto &conflict = conflicts[index];
            lession_id_proxy.emplace(Lession::registery.entity(entt),&copy);
            copy.name = origin.name.c_str();
            copy.hour = origin.hours + hour_offset;
            copy.semister = origin.semister + 1;
            conflict.lession = &copy;
            index++;
        }
        // initialize lessions require & update conflicts
        progress=0.1;
        for(auto entt:lview) {
            const auto &value = lview.get(entt);
            if(Array::null != value.require)
            {
                EnchantLession *require  = lession_id_proxy.at(Lession::registery.entity(entt));
                EnchantLession *required = lession_id_proxy.at(Lession::registery.entity(value.require));
                // both gets conflict count
            #ifdef _MSC_VER
                conflicts[require - lessions.data()].count++;
                conflicts[required - lessions.data()].count++;
            #else
                conflicts[require - lessions].count++;
                conflicts[required- lessions].count++;
            #endif
                require->set_conflict(required,Scores::conflict_requires);
                required->set_conflict(require,Scores::conflict_requiredby);
            }
        }
        // initialize groups & update conflicts
        progress=0.15;
        index=0;
        auto gview = Group::registery.view();
        for(auto entt:gview)
        {
            const Group &origin = gview.get(entt);
            for(auto member : origin.members)
            {
                EnchantLession *lession_pointer = lession_id_proxy.at(Lession::registery.entity(member));
                // each lession get one conflict count per each group it joint
            #ifdef _MSC_VER
                conflicts[lession_pointer - lessions.data()].count++;
            #else
                conflicts[lession_pointer - lessions].count++;
            #endif
                for(auto other : origin.members)
                {
                    const EnchantLession *other_pointer = lession_id_proxy.at(Lession::registery.entity(other));
                    if(other_pointer != lession_pointer)
                    {
                        lession_pointer->set_conflict(other_pointer,(origin.priority+1)*Scores::conflict_group_multiplier);
                    }
                }
            }
        }
        // initialize professors
        progress=0.2;
        index=0;
        auto pview = Professor::registery.view();
        for(auto entt:pview) {
            const Professor &origin = pview.get(entt);
            auto &buffer = professors_buffer[index];
            auto &pointer_dynamics = professors_dynamic[index];
            professor_id_proxy.emplace(Professor::registery.entity(entt),&buffer);
            buffer.name = origin.name.c_str();
            for (size_t i = 0; i < 25; i++)
            {
                if (origin.hours[i]){
                    buffer.hours[i*2]=&freetime;
                    buffer.hours[i*2+1]=&freetime;
                }else{
                    buffer.hours[i*2]=nullptr;
                    buffer.hours[i*2+1]=nullptr;
                }
            }
            for (size_t i = 0; i < 25; i++)
            {
                if (origin.uncertain_hours[i]){
                    pointer_dynamics.dynamic_hours[i*2]=1;
                    pointer_dynamics.dynamic_hours[i*2+1]=1;
                }else{
                    pointer_dynamics.dynamic_hours[i*2]=0;
                    pointer_dynamics.dynamic_hours[i*2+1]=0;
                }
            }
            // optimize it away.
            pointer_dynamics.max_dynamic_hours = origin.max_uncertain_hours;
            index++;
        }
        // pair lessions with professors
        progress=0.25;
        index=0;
        for(const auto &pair : Pair::map)
        {
            EnchantLession *less = lession_id_proxy.at(Lession::registery.entity(pair.first));
            ProfessorBuffer *prof  = professor_id_proxy.at(Professor::registery.entity(pair.second));
            less->assigned_professor = prof;
        }
    }
    // sort by most conflicts.
    progress=0.30;
#ifdef _MSC_VER
    rsort({conflicts.data(),lession_count});
#else
    rsort({conflicts,lession_count});
#endif
    // call each lession by most conflicts.
    TimeTable timetable{};
    progress=0.40;
    for (const ConflictBuffer& lession : conflicts) {
        recursive_solve(*lession.lession, timetable);
    }
    print_result(timetable);
}catch(const std::exception& e){
    terminal.append(e.what());
}progress=1.0;}


void apply_result(ScoreStack &res,TimeTable &timetable) {
    while (!res.choice_stack.empty())
    {
        const Choice& value = res.choice_stack.top();
        const int index = value.position;
        const EnchantLession* new_value = value.lession;
        if (index < 0 || index > 49)
            throw std::runtime_error("undefined hour");
        span<EnchantLession*> prof_hours{ value.lession->assigned_professor->hours,50 };
        // update new positions
        EnchantLession* old_value = prof_hours[index];
        timetable.hours[index].erase(old_value);
        prof_hours[index] = (EnchantLession*)new_value;
        timetable.hours[index].emplace((EnchantLession*)new_value);
        res.choice_stack.pop();
    }
}

void recursive_solve(const EnchantLession& lession,TimeTable &timetable) {
    if (lession.hour == 1 || lession.hour == 2) {
        ScoreStack res;
        conflict_function_t conflict_function =
            lession.hour == 2 ? recursive_conflicts2 : recursive_conflicts1;
        res = conflict_function(
            timetable,
            lession,
            *lession.assigned_professor,
            SERACH_DEPTH
        );
        if (res.total > Scores::conflict_max)
            throw std::runtime_error("no solution was found");
        if(((const Choice&)res.choice_stack.top()).lession != &lession)
            throw std::runtime_error("unexpected solution");
        apply_result(res, timetable);
    } else if (lession.hour == 3) {
        {
            ScoreStack res = recursive_conflicts2(
                timetable,
                lession,
                *lession.assigned_professor,
                SERACH_DEPTH
            );
            if (res.total > Scores::conflict_max)
                throw std::runtime_error("no solution was found");
            if (((const Choice&)res.choice_stack.top()).lession != &lession)
                throw std::runtime_error("unexpected solution");
            apply_result(res, timetable);
        }

        ProfessorBuffer professor_new(*lession.assigned_professor);
        for (auto &hour: professor_new.hours) {
            if (hour == &lession)
                hour = nullptr;
        }

        {
            ScoreStack res = recursive_conflicts1(
                timetable,
                lession,
                professor_new,
                SERACH_DEPTH
            );
            if (res.total > Scores::conflict_max)
                throw std::runtime_error("no solution was found");
            const Choice& last_choice = res.choice_stack.top();
            if (last_choice.lession != &lession)
                throw std::runtime_error("unexpected solution");
            professor_new.hours[last_choice.position] = nullptr;
            apply_result(res, timetable);
        }
    }
    else
        throw std::runtime_error("unexpected lession::hour");
}


void print_result(const TimeTable &timetable) {
    for (int i = 0; i < 50; i += 2) {
        auto& day_even = timetable.hours[i];
        auto& day_odd = timetable.hours[i + 1];
        for (const EnchantLession* c : day_even) {
            if (day_odd.count(c))
                printf("{both days: %d, lession: %s, professot: %s}\n", i / 2, c->name, c->assigned_professor->name);
            else
                printf("{even days: %d, lession: %s, professot: %s}\n", i / 2, c->name, c->assigned_professor->name);
        }
        for (const EnchantLession* c : day_odd) {
            if (!day_even.count(c))
                printf("{ odd days: %d, lession: %s, professot: %s}\n", i / 2, c->name, c->assigned_professor->name);
        }
    }
}

void rsort(span<ConflictBuffer> array) {
    // ConflictBuffer temp[array.size()];
    // memcpy (array.data(), temp, array.size_bytes());
    for (size_t i = 0; i < array.size(); i++)
        for (size_t j = i+1; j < array.size(); j++)
            if(array[j].count > array[i].count)
                std::swap(array[j],array[i]);
}


//calculte conflict score,
//bitween given lession and array of other lessions,
//if there was any.
//note: ignores lessions with same professor.
Score other_conflicts(const std::unordered_set<const EnchantLession *> &array,const EnchantLession &lession){
    //
    Score total = 0;
    for(const  EnchantLession *other_lession:array) {
        if(other_lession->assigned_professor == lession.assigned_professor)
            continue;
        if(other_lession == &freetime)
            throw std::runtime_error("unexpected freetime in timetable");
        const auto res = lession.conflicts.find(other_lession);
        const auto end = lession.conflicts.end();
        if(res != end)
            total += res->second;
    }
    return total;
}

// returns least conflict Score
ScoreStack recursive_conflicts1 (
    const TimeTable &timetable,
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
                result_temp.choice_stack.emplace(&lession,i);
            }
            else if(hour == &freetime)
            {
                result_temp.total = Scores::conflict_free_hour;
                result_temp.choice_stack.emplace(&lession,i);
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
                    result_temp = recursive_conflicts2(timetable, *hour, professor_temp, level - 1);
                }else{
                    professor_temp.hours[i] = nullptr;
                    result_temp = recursive_conflicts1(timetable, *hour, professor_temp, level - 1);
                }
                if(result_temp.total > Scores::conflict_max)
                        continue;
                result_temp.choice_stack.emplace(&lession,i);
                result_temp.total += Scores::conflict_lession_swap;
            }
            result_temp.total += other_conflicts(timetable.hours[i],lession);
            best_result.swap_if_better(result_temp);
        }
    }
    return best_result;
}

// returns least conflict Score
ScoreStack recursive_conflicts2(
    const TimeTable& timetable,
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
                result_temp.choice_stack.emplace(&lession, i);
                result_temp.choice_stack.emplace(&lession, i + 1);
            }
            else if (hour1 == &freetime && hour2 == &freetime)
            {
                result_temp.total = Scores::conflict_free_hour * 2;
                result_temp.choice_stack.emplace(&lession, i);
                result_temp.choice_stack.emplace(&lession, i + 1);
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
                    result_temp = recursive_conflicts2(timetable, *hour1, professor_temp, level - 1);
                    if (result_temp.total > Scores::conflict_max)
                        continue;
                    result_temp.choice_stack.emplace(&lession, i);
                    result_temp.choice_stack.emplace(&lession, i + 1);
                    result_temp.total += Scores::conflict_lession_swap * 2;
                }
                else if (hour1 && hour1 != &freetime && hour2 && hour2 != &freetime)
                {
                    ProfessorBuffer professor_temp(professor);
                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i + 1] = nullptr;
                    result_temp = recursive_conflicts1(timetable, *hour1, professor_temp, level - 1);
                    if (result_temp.total > Scores::conflict_max)
                        continue;
                    ScoreStack result_temp2 = recursive_conflicts1(timetable, *hour1, professor_temp, level - 1);
                    if (result_temp2.total > Scores::conflict_max)
                        continue;
                    result_temp.choice_stack.emplace(&lession, i);

                    while (!result_temp2.choice_stack.empty()) {
                        result_temp.choice_stack.emplace(result_temp2.choice_stack.top());
                        result_temp2.choice_stack.pop();
                    }

                    result_temp.choice_stack.emplace(&lession, i + 1);
                    result_temp.total += result_temp2.total;
                    result_temp.total += Scores::conflict_lession_swap * 2;
                }
                else
                {
                    if (hour2 != &freetime)
                        hour1 = hour2;
                    ProfessorBuffer professor_temp(professor);
                    professor_temp.hours[i] = nullptr;
                    professor_temp.hours[i + 1] = nullptr;
                    result_temp = recursive_conflicts1(timetable, *hour1, professor_temp, level - 1);
                    if (result_temp.total > Scores::conflict_max)
                        continue;
                    result_temp.choice_stack.emplace(&lession, i);
                    result_temp.choice_stack.emplace(&lession, i + 1);
                    result_temp.total += Scores::conflict_lession_swap + Scores::conflict_free_hour;
                }
            }
            result_temp.total += other_conflicts(timetable.hours[i], lession);
            result_temp.total += other_conflicts(timetable.hours[i + 1], lession);
            best_result.swap_if_better(result_temp);
        }
    }
    return best_result;
}
