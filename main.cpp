#include "Solver.hpp"
#include "uni.hpp"
#include <iostream>
float _=0.0f;
int WinMain()
{
    advanced_array::entity prof1_id,prof2_id,
        less1_id,less2_id,less3_id,less4_id,less5_id,less6_id,
        grup1_id;

    prof1_id = Professor::registery.create();
    {
        Professor &prof1 = Professor::registery.emplace(prof1_id,prof1_id,"Prof1");
        prof1.hours[0]=1;
        prof1.hours[1]=1;
        prof1.hours[2]=1;
        prof1.hours[3]=1;
    }
    prof2_id = Professor::registery.create();
    {
        Professor &prof1 = Professor::registery.emplace(prof2_id,prof2_id,"Prof2");
        prof1.hours[0]=1;
        prof1.hours[1]=1;
        prof1.hours[2]=1;
        prof1.hours[3]=1;
    }

    less1_id = Lession::registery.create();
    Lession::registery.emplace(less1_id,less1_id,"Math1");
    less2_id = Lession::registery.create();
    Lession::registery.emplace(less2_id,less2_id,"Math2");
    less3_id = Lession::registery.create();
    Lession::registery.emplace(less3_id,less3_id,"Discrite");
    less4_id = Lession::registery.create();
    Lession::registery.emplace(less4_id,less4_id,"Differential");
    less5_id = Lession::registery.create();
    Lession::registery.emplace(less5_id,less5_id,"ETC1");
    less6_id = Lession::registery.create();
    Lession::registery.emplace(less6_id,less6_id,"ETC2");

    {
        Lession &less2 = Lession::registery.get(less2_id);
        Lession &less3 = Lession::registery.get(less3_id);
        Lession &less4 = Lession::registery.get(less4_id);
        less2.require = less1_id;
        less3.require = less1_id;
        less4.require = less2_id;
    }
    grup1_id = Group::registery.create();
    {
        Group &group1 = Group::registery.emplace(grup1_id,grup1_id,"Grup1");
        group1.priority = 1;
        group1.members.emplace_back(less4_id);
        group1.members.emplace_back(less5_id);
    }

    Pair::create(less1_id,prof1_id);
    Pair::create(less2_id,prof1_id);
    Pair::create(less4_id,prof1_id);
    Pair::create(less3_id,prof2_id);
    Pair::create(less5_id,prof2_id);
    Pair::create(less6_id,prof2_id);

    solve_function(_);
    return 0;
}
