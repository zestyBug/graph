#include "Core.hpp"





#include "imgui/imgui.h"
//#include <iostream>
//#include <memory>
//#include <mutex>
//#include <map>
//#include <array>
//#include <vector>
//#include <string>
#include <Window.h>
#include <thread>

#include "uni_ini.hpp"

#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  undef APIENTRY

#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
# endif
#  include <windows.h>
#endif


#include "uni.hpp"
#include "ui_components.hpp"













const ImGuiWindowFlags window_flags =
    //ImGuiWindowFlags_NoDecoration |
    //ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoScrollbar |
    //ImGuiWindowFlags_NoCollapse |
    //ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize |
    //ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_MenuBar
    ;


// *10*212#





Core::Core(){}
Core::~Core(){}
// before initializing the engine
int Core::OnInit()
{
    return 0;
}
// after creating rendering context
int Core::OnCreate()
{
    return 0;
}
// before starting to render
int Core::OnStart()
{
    return 0;
}
// actual rendering process
int Core::OnDraw(double delta,bool dirty)
{
    //if(show_demo)ImGui::ShowDemoWindow(&show_demo);


    if(1)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
    }

    if (ImGui::Begin("This text should not be displayed!", &show_window, window_flags))
	{
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File##Menu"))
            {
                if (ImGui::MenuItem("About##Menu", NULL, false, true))
                    show_demo = true;
                if (ImGui::MenuItem("Save##Menu", NULL, false, true))
                    save_ini("result.txt");
                if (ImGui::MenuItem("Load##Menu", NULL, false, true))
                    load_ini("result.txt");
                ImGui::MenuItem("Terminal##Menu", NULL, &show_terminal);
                if(!timetable)
                    if(ImGui::MenuItem("Build##Menu", NULL, false, true)) {
                        timetable = new TimeTable();
                    }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImVec2 window_size = ImVec2(0, 0);
        if (show_terminal)
            window_size = ImVec2(0, -terminal_size);

        ImGui::BeginChild("MyTabBar_layout", window_size);
        if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None))
        {
            if (this->timetable && this->timetable->sthread.joinable()) {
                ImGui::ProgressBar(this->timetable->progress, ImVec2(0.0f, 0.0f));
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::Text("Progress");
                if (this->timetable->progress >= 1.0f) {
                    if (ImGui::Button("OK"))
                        this->timetable->sthread.join();
                }
            } else if(this->timetable) {
                if (this->timetable->panel()) {
                    delete this->timetable;
                    this->timetable = nullptr;
                }
            } else {
                if (ImGui::BeginTabItem("Professor"))
                {
                    professor_tab();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Lessons"))
                {
                    lession_tab();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Groups"))
                {
                    group_tab();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Relation"))
                {
                    relation_tab();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Settings"))
                {
                    setting_tab();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
        if (show_terminal)
            terminal_tab();
	}

    if (show_demo)
    {
        const bool res = ImGui::BeginPopup("About##Popup", ImGuiWindowFlags_Popup);
        if (res)
        {
            ImGui::Text("Simple timetable application (stable v0.1)");
            ImGui::Separator();
            ImGui::Text("By Zia Sahebi.");
            ImGui::Text("Libraries used: ImGui %s, QuickJS 2024-02-14, ENTT 3.13.2.", ImGui::GetVersion());
            ImGui::EndPopup();
        }
        else
        {
            //show_demo = false;
        }
    }

	ImGui::End();

    (void)delta;
    (void)dirty;
    return !show_window;
}
// after stoping render (interupted by user)
int Core::OnStop()
{
    return 0;
}
// before destroing context and quiting
void Core::OnDestroy()
{
}




