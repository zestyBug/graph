#include "uni.hpp"
#include "ui_components.hpp"
#include <array>
#include "span.hpp"
#include "range.hpp"
#include "Solver.hpp"

Array::registry<Group> Group::registery;
Array::registry<Professor> Professor::registery;
Array::registry<Lession> Lession::registery;
std::map<Array::entity,Array::entity> Pair::map;
std::array<bool[25],8> Setting::forbiden;
std::string terminal;

static EnchantLession freetime;


void solve_function(float& progress)
{try{
    const auto lession_count = Lession::registery.pool_size();
    const auto group_count = Group::registery.pool_size();
    const auto professor_count = Professor::registery.pool_size();
    if(lession_count==0 || professor_count==0)
        throw std::range_error("unexpected zero lession or professor count");
    // list of lessions
    // last one represent free time
    EnchantLession lessions[lession_count+1];
    // count of conflicts with a refrence to the lession
    ConflictBuffer conflicts[lession_count];
    // groups with their member
    EnchantGroup groups[group_count];
    // store constant information
    EnchantProfessor professors[professor_count];
    // stores current professors time decisions
    ProfessorBuffer professors_buffer[professor_count];
    // stores professors dynamic time information
    EnchantProfessorDynamic professors_dynamic[professor_count];

    {
        std::map<Array::entity,EnchantLession*> lession_id_proxy;
        std::map<Array::entity,EnchantGroup*> group_id_proxy;
        std::map<Array::entity,EnchantProfessor*> professor_id_proxy;

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
            copy.hour = origin.hours + hour_offset;
            copy.semister = origin.semister + 1;
            conflict.lession = &copy;
            index++;
        }
        // initialize lessions require & update conflicts
        progress=0.1;
        for(auto entt:lview) {
            const auto &value = lview.get(entt);
            if(Array::null != value.require) {
                EnchantLession *require  = lession_id_proxy.at(Lession::registery.entity(entt));
                EnchantLession *required = lession_id_proxy.at(Lession::registery.entity(value.require));
                // both gets conflict count
                conflicts[require - lessions].count++;
                conflicts[required- lessions].count++;
                require->require = required;
                required->required_by.emplace_back(require);
            }
        }
        // initialize groups & update conflicts
        progress=0.15;
        index=0;
        auto gview = Group::registery.view();
        for(auto entt:gview) {
            const auto &origin = gview.get(entt);
            auto &copy = groups[index++];
            copy.members.reserve(origin.members.size());
            for(auto member : origin.members){
                EnchantLession *lession_pointer = lession_id_proxy.at(Lession::registery.entity(member));
                // each lession get one conflict count per each group it joint
                conflicts[lession_pointer - lessions].count++;
                copy.members.emplace_back(lession_pointer);
            }
            copy.score = (origin.priority+1)*Scores::group_multiplier;
        }
        // initialize professors
        progress=0.2;
        index=0;
        auto pview = Professor::registery.view();
        for(auto entt:pview) {
            const Professor &origin = pview.get(entt);
            auto &copy = professors[index];
            auto &buffer = professors_buffer[index];
            auto &pointer_dynamics = professors_dynamic[index];
            professor_id_proxy.emplace(Professor::registery.entity(entt),&copy);
            for (size_t i = 0; i < 25; i++)
            {
                if (origin.hours[i]){
                    buffer.hours_score[i*2]=&freetime;
                    buffer.hours_score[i*2+1]=&freetime;
                    copy.hours_score[i*2]=Scores::idle_hour;
                    copy.hours_score[i*2+1]=Scores::idle_hour;
                }else{
                    buffer.hours_score[i*2]=nullptr;
                    buffer.hours_score[i*2+1]=nullptr;
                    copy.hours_score[i*2]=Scores::unavailable_hour;
                    copy.hours_score[i*2+1]=Scores::unavailable_hour;
                }
            }
            for (size_t i = 0; i < 25; i++)
            {
                if (origin.uncertain_hours[i]){
                    pointer_dynamics.dynamic_hours[i*2]=Scores::idle_hour;
                    pointer_dynamics.dynamic_hours[i*2+1]=Scores::idle_hour;
                }else{
                    pointer_dynamics.dynamic_hours[i*2]=Scores::unavailable_hour;
                    pointer_dynamics.dynamic_hours[i*2+1]=Scores::unavailable_hour;
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
            EnchantProfessor *prof  = professor_id_proxy.at(Professor::registery.entity(pair.second));
            less->assigned_professor = prof;
            prof->assigned_lessions.emplace_back(less);
        }
    }
    // sort by most conflicts.
    progress=0.30;
    rsort({conflicts,lession_count});
    // call each lession by most conflicts.
    TimeTable timetable{};
    progress=0.40;
    for(auto &lession:conflicts) {
        //
    }
}catch(const std::exception& e){
    terminal.append(e.what());
}progress=1.0;}


void rsort(span<ConflictBuffer> array) {
    // ConflictBuffer temp[array.size()];
    // memcpy (array.data(), temp, array.size_bytes());
    for (size_t i = 0; i < array.size(); i++)
        for (size_t j = i+1; j < array.size(); j++)
            if(array[j].count > array[i].count)
                std::swap(array[j],array[i]);
}



struct LessionBuffer {
    // 1 ~ 50
    // 0: not used
    char resrved_hours[4];
    LessionBuffer() {
        memset(resrved_hours,0,sizeof(resrved_hours));
    }
};
struct WeekMask
{
    int hours_score[5*5*2];
    WeekMask() {
        memset(hours_score,0,sizeof(hours_score));
    }
};

int Score(
    const span<const EnchantLession> lessions,
    const span<const EnchantProfessor> professors,
    const span<const EnchantProfessorDynamic> dynamincs,
    const span<const EnchantGroup> group
){
    for (size_t semister=8;semister>0;semister--)
        for(auto &lession:lessions)
            if(lession.semister == semister){
                WeekMask mask;
            }
    return 0;
}