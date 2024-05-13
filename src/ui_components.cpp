#include "ui_components.hpp"
#include "imgui/imgui.h"



const float terminal_size = 100.0f;



bool del_btn(){
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.9f, 0.9f));
    bool res = ImGui::Button("Delete this");
    ImGui::PopStyleColor(3);
    return res;
}






void professor_tab()
{
    auto &professor = Professor::panel();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()-5)); // Leave room for 1 line below us

    if(&professor){
        professor.print();
    }

    ImGui::EndChild();
    ImGui::Separator();
    if (ImGui::Button("Revert")){
        if(&professor){
            memset(professor.hours,0,sizeof(Professor::hours));
            memset(professor.uncertain_hours,0,sizeof(Professor::uncertain_hours));
            professor.max_uncertain_hours=0;
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Unmark check boxes");
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
        Professor::create("New Professor");
    }
    ImGui::SameLine();
    if (del_btn()) {
        if(&professor){
            Array::entity result = Professor::del(professor);
            if(Array::null != result){
                char n[32];
                terminal.append("Error: conflicf with #");
                snprintf(n,31,"%lu\n",result);
                terminal.append(n);
            }
        }
    }
    ImGui::EndGroup();
}






void lession_tab()
{
    Lession &lession = Lession::panel();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()-5)); // Leave room for 1 line below us

    if(&lession){
        lession.print();
    }

    ImGui::EndChild();
    ImGui::Separator();
    if (ImGui::Button("Add")) {
        Lession::create("New Lession");
    }
    ImGui::SameLine();
    if (del_btn()) {
        if(&lession){
            Array::entity result = Lession::del(lession);
            if(Array::null != result){
                char n[32];
                terminal.append("Error: conflicf with #");
                snprintf(n,31,"%lu\n",result);
                terminal.append(n);
            }
        }
    }
    ImGui::EndGroup();
}






void group_tab()
{
    Group &group = Group::panel();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()-5)); // Leave room for 1 line below us

    if(&group){
        group.print();
    }

    ImGui::EndChild();
    ImGui::Separator();
    if (ImGui::Button("Add")) {
        Group::create("New Group");
    }
    ImGui::SameLine();
    if (del_btn()) {
        if(&group){
            Array::id_type result = Group::del(group);
            if(Array::null != result){
                char n[32];
                terminal.append("Error: conflicf with #");
                snprintf(n,31,"%lu\n",result);
                terminal.append(n);
            }
        }
    }
    ImGui::EndGroup();
}




void relation_tab()
{
    const ImVec4 color = ImVec4(0.9f,0.9f,0.9f,0.9f);
    ImGui::TextColored(color,"Error");
    ImGui::SameLine();
    ImGui::Text("CoominSoon.");
    ImGui::Text("Application average %.3f (%.5g) ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime, ImGui::GetIO().Framerate);
}

void setting_tab()
{
    static int term = 0;
    ImGui::Combo("priority", &term,"Term 1\0Term 2\0Term 3\0Term 4\0Term 5\0Term 6\0Term 7\0Term 8\0");
    ImGui::Text("Forbidden Days");
    Weekbox(Setting::forbiden.at((size_t)term));
}



void terminal_tab()
{
    ImGui::InputTextMultiline("##terminal_tab", terminal.data(), terminal.length(), ImVec2(-FLT_MIN, -ImGui::GetStyle().FramePadding.y/2.0f /* ImGui::GetTextLineHeight() * 16 */), ImGuiInputTextFlags_ReadOnly);
}
