#ifndef CORE_HPP
#define CORE_HPP 1

#include "Solver.hpp"
#include <thread>

// a Core class is like activity class in Android.
// should have no static variable,
// because may multiple instances be created.
class Core
{
protected:
    bool show_window=true,show_demo=false, show_terminal=true;
    std::thread sthread;
    TimeTable* timetable = nullptr;
    void *context=nullptr;
    float progress=0.0f;
public:
    Core();
    ~Core();
    // before initializing the engine
    int OnInit();
    // after creating rendering context
    int OnCreate();
    // before starting to render
    int OnStart();
    // actual rendering process,
    // returns 1 if changes happened and a swap required,
    // throws exeption on error
    int OnDraw(double delta,bool dirty);
    // after stoping render (interupted by user)
    int OnStop();
    // before destroing context and quiting
    void OnDestroy();
};


#endif
