#ifndef UNI_H
#define UNI_H

#include <string>
#include <string.h>
#include <vector>
#include <map>
#include "advanced_array/advanced_array.hpp"

namespace Array = advanced_array;


void Weekbox(bool * const check_list);

struct Base {
    Base(Array::entity _id,const char *_name):id(_id),name(_name) {
        name.reserve(64);
    }
    Array::entity id;
    std::string name;
};

struct Professor : public Base
{
    friend class Pair;
    friend class Lession;
    static Array::registry<Professor> registery;
    Professor(Array::entity _id,const char *_name):Base(_id,_name),max_uncertain_hours{0} {
        memset(hours,0,sizeof(hours));
        memset(uncertain_hours,0,sizeof(uncertain_hours));
    }
    bool hours[25];
    int max_uncertain_hours;
    bool uncertain_hours[25];
public:
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


struct Lession : public Base
{
    friend class Pair;
    friend class Group;
    friend class Professor;
    static char const * const hours_lable[3];
    static Array::registry<Lession> registery;
    Lession(Array::entity _id,const char *_name)
        :Base(_id,_name),hours{1},semister{1},require{Array::null}
    {}
    // options: 0,1,2
    int hours;
    // options: 0 ~ 7
    int semister;
    Array::entity require;
public:
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


struct Group : public Base
{
    static Array::registry<Group> registery;
    static char const * const priorities[3];
    std::vector<Array::entity> members;
    // options: 0,1,2
    int priority;
    Group(Array::entity _id, const char * _name):Base(_id,_name),members(),priority{1} {
        members.reserve(10);
    }
public:
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


struct Pair {
    friend class Professor;
    static std::map<Array::entity,Array::entity> map;
public:
    static void create(Array::entity l, Array::entity t) {
        map.emplace(l,t);
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
            return search->first;
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

struct Setting {
    static std::array<bool[25],8> forbiden;
};

void solve_function(float& progress);


#endif // UNI_H
