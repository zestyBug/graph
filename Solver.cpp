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
    const auto professor_count = Professor::registery.pool_size();
    if(lession_count==0 || professor_count==0)
        throw std::range_error("unexpected zero lession or professor count");
    // list of lessions
    // last one represent free time
    EnchantLession lessions[lession_count+1];
    // count of conflicts with a refrence to the lession
    ConflictBuffer conflicts[lession_count];
    // stores current professors time decisions
    ProfessorBuffer professors_buffer[professor_count];
    // stores professors dynamic time information
    EnchantProfessorDynamic professors_dynamic[professor_count];

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
                conflicts[require - lessions].count++;
                conflicts[required- lessions].count++;
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
                conflicts[lession_pointer - lessions].count++;
                for(auto other : origin.members)
                {
                    const EnchantLession *other_pointer = lession_id_proxy.at(Lession::registery.entity(other));
                    if(other_pointer != lession_pointer)
                    {
                        lession_pointer->set_conflict(other_pointer,(origin.priority+1)*Scores::group_multiplier);
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
                    buffer.hours_score[i*2]=&freetime;
                    buffer.hours_score[i*2+1]=&freetime;
                }else{
                    buffer.hours_score[i*2]=nullptr;
                    buffer.hours_score[i*2+1]=nullptr;
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
    rsort({conflicts,lession_count});
    // call each lession by most conflicts.
    TimeTable timetable{};
    progress=0.40;
    for(const ConflictBuffer &lession:conflicts) {
        printf("{'name': '%s', 'conflicts': %u}\n",lession.lession->name,lession.count);
        // in teacher non-unavailable (non nullptr) times
        // search for a place with least amount of conflict
        // search proccess must be recursive
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



