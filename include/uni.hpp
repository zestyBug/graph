#ifndef UNI_H
#define UNI_H

#include <string>
#include <cstring>
#include <vector>
#include <array>
#include <map>
#include "advanced_array/advanced_array.hpp"
#include "quickjs/quickjs.h"
#include "Solver.hpp"


namespace Array = advanced_array;

#define MAIN_FUNCTION int main()
//std::initializer_list could be better
extern char const * const  lable_name[25];

void Weekbox(bool * const check_list);

struct Base {
    Base(Array::entity _id,const char *_name):id(_id),name(_name) {
        name.reserve(64);
    }
    Array::entity id;
    std::string name;
};

class Professor : public Base
{
protected:
    friend class Pair;
    friend class Lession;
    friend class TimeTable;
    friend void professor_tab();
    friend JSValue js_uni_new_professor(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
    friend JSValue js_uni_new_lession(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
    static Array::registry<Professor> registery;
    // 0~24
    // true: available
    bool hours[25];
    int max_uncertain_hours;
    bool uncertain_hours[25];
public:
    Professor(Array::entity _id,const char *_name):Base(_id,_name),max_uncertain_hours{0} {
        memset(hours,0,sizeof(hours));
        memset(uncertain_hours,0,sizeof(uncertain_hours));
    }
    static Professor& create(const char *name)
    {
        Array::entity obj = registery.create();
        return registery.emplace(obj, obj, name);
    }
    static Array::entity del(Professor& obj);
    // return entt::null on success
    // o.w. id of lession it conflict with
    void print();
    void list();
    // select a Professor for the given lession id,
    // can overwrite current one.
    static void popup_select(Array::entity lession);
    static Professor& panel();
};


class Lession : public Base
{
protected:
    friend class Pair;
    friend class Group;
    friend class Professor;
    friend class TimeTable;
    friend JSValue js_uni_new_group(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
    friend JSValue js_uni_new_lession(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
    static char const * const hours_lable[3];
    static Array::registry<Lession> registery;
    // options: 
    // 0 => 2 hours per 2 week
    // 1 => 3 hours per 2 week
    // 2 => 4 hours per 2 weeks
    int hours;
    // set how it conflicts acoording to semister of other lessions
    // normal: 1 ~ 8
    // not specified: < 1
    // all: > 8
    // see 'other_conflicts' for more info.
    int semister;
    // gives the lession periority to be placed in timetable.
    int weight;
    Array::entity require;
public:
    Lession(Array::entity _id, const char* _name)
        :Base(_id, _name), hours{ 1 }, semister{ -1 }, weight{ 0 }, require {Array::null}
    {}
    static Lession& create(const char *name) {
        Array::entity obj = registery.create();
        return registery.emplace(obj, obj, name);
    }
    // return entt::null on success
    // o.w. id of lession it conflict with
    static Array::entity del(Lession& obj);
    void print();
    static void list(Array::entity entity);
    void print_required();
    void print_professor() const;
    // add a lession for the given professor id,
    static void popup_add(Array::entity professor);
    // select required lession for the this object,
    // can overwrite current one.
    static void popup_require(Lession &lession);
    static Lession& panel();
};


class Group : public Base
{
protected:
    friend class TimeTable;
    friend JSValue js_uni_new_group(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
    static Array::registry<Group> registery;
    static char const * const priorities[3];
    std::vector<Array::entity> members;
    // options: 0,1,2
    int priority;
public:
    Group(Array::entity _id, const char * _name):Base(_id,_name),members(),priority{1} {
        members.reserve(10);
    }
    static Group& create(const char *name) {
        Array::entity obj = registery.create();
        return registery.emplace(obj, obj, name);
    }
    // return entt::null on success
    // o.w. id of lession it conflict with
    static Array::entity del(Group& obj) {
        registery.destroy(obj.id);
        return Array::null;
    }
    static Array::entity isInAny(const Lession& lession)
    {
        auto view = registery.view();
        for(auto ent:view) {
            Group& group = view.get(ent);
            for(auto member:group.members)
                if(member == lession.id)
                    return ent;
        }
        return Array::null;
    }
    void print();
    void list();
    void popup_add();
    static Group& panel();
};


class Pair {
protected:
    friend class Professor;
    friend class TimeTable;
    static std::map<Array::entity,Array::entity> map;
public:
    static void create(Array::entity l, Array::entity t) {
        map.emplace(l,t);
    }
    static void create(const Lession& l, const Professor& t) {
        map.emplace(l.id,t.id);
    }
    // return entt::null on success
    // o.w. id of lession it conflict with
    static Array::entity del(Array::entity l) {
        map.erase(l);
        return Array::null;
    }
    // return entt::null on success
    // o.w. id of lession it conflict with
    static Array::entity deleteByLession(const Lession& lession) {
        map.erase(lession.id);
        return Array::null;
    }
    static Array::entity findProfessor(const Lession& lession){
        auto search = map.find(lession.id);
        if (search != map.end())
            return search->second;
        return Array::null;
    }
    static Array::entity isInAny(const Professor& professor)
    {
        for (auto it = map.begin(); it != map.end(); ++it)
            if(it->second == professor.id)
                return it->first;
        return Array::null;
    }
    static void assign(Array::entity lession, Array::entity professor){
        map.emplace(lession,professor);
    }
};

namespace Setting {
    extern std::array<bool[25],8> forbiden;
};

void solve_function(float& progress);


#endif // UNI_H
