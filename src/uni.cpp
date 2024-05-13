#include "uni.hpp"
#include "imgui/imgui.h"

char const * const Group::priorities[3] = {"Low","Meduim","High"};
char const * const Lession::hours_lable[3] = {"2 Hours","3 Hours","4 Hours"};
static char const * const days_lable[5] = {"sat","sun","mon","tue","wed"};
char forbidden_days[25];

char const * const  lable_name[25] = {
    "sat 08:30", "sat 10:30", "sat 13:00", "sat 15:30", "sat 17:00",
    "sun 08:30", "sun 10:30", "sun 13:00", "sun 15:30", "sun 17:00",
    "mon 08:30", "mon 10:30", "mon 13:00", "mon 15:30", "mon 17:00",
    "tue 08:30", "tue 10:30", "tue 13:00", "tue 15:30", "tue 17:00",
    "wed 08:30", "wed 10:30", "wed 13:00", "wed 15:30", "wed 17:00",
};

void Weekbox(bool * const check_list)
{
    for(int i=0;i<25;i++)
    {
        char const * const lable = lable_name[i];
        ImGui::Checkbox2(lable, &check_list[i]);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(lable);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        if((i%5)!=4)
            ImGui::SameLine();
    }
}

static void info(const char *TEXT) {
    ImGui::TextDisabled("(i)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(TEXT);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

Array::entity Professor::del(Professor& obj)
{
    Array::entity lession = Pair::isInAny(obj);
    if(Array::null == lession)
        registery.destroy(obj.id);
    return lession;
}

void Professor::print()
{
    ImGui::Text("#%lu",this->id);
    ImGui::InputText("##name",this->name.data(),this->name.capacity()-1);
    ImGui::Text("Schedule");
    Weekbox(this->hours);
    ImGui::BeginDisabled();
    ImGui::Text("Uncertainty");
    ImGui::InputInt("max uncertain days", &this->max_uncertain_hours, 1, 1);
    this->max_uncertain_hours = max(0,this->max_uncertain_hours);
    this->max_uncertain_hours = min(5,this->max_uncertain_hours);
    Weekbox(this->uncertain_hours);
    ImGui::EndDisabled();
    ImGui::Text("Assigned Lessons");
    ImGui::SameLine();
    if (ImGui::Button("Add lession"))
        ImGui::OpenPopup("Select a lession##1");
    if (ImGui::BeginPopupModal("Select a lession##1", NULL, 0))
    {
        Lession::popup_add(this->id);
        ImGui::EndPopup();
    }
    ImGui::Indent(1.0f);{
        this->list();
    }ImGui::Unindent(1.0f);
}
void Professor::list()
{
    for (auto it = Pair::map.begin(); it != Pair::map.end(); ++it)
        if(it->second == this->id)
            Lession::list(it->first);
}
void Professor::popup_select(Array::entity lession){
    static Array::entity popup_selected = Array::null;
    ImGui::BeginChild("lesson_popup", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()-5), true);
    auto view = registery.view();
    for(auto ent: view) {
        auto &prof =  view.get(ent);
        ImGui::PushID(&prof);
        if (ImGui::Selectable(prof.name.c_str(), popup_selected == ent))
            popup_selected = ent;
        ImGui::PopID();
    }
    ImGui::EndChild();

    bool cc = ImGui::Button("Cancel");
    ImGui::SameLine();
    bool ok = ImGui::Button("OK");
    if(ok && Array::null != popup_selected)
        Pair::assign(lession,popup_selected);
    if (ok || cc)
        ImGui::CloseCurrentPopup();
}
Professor& Professor::panel()
{
    static Array::entity selected = Array::null;
    ImGui::BeginChild("leftpane", ImVec2(150, 0), true);
    auto view = registery.view();
    // use forward iterators and get only the components of interest
    for(Array::entity ent: view) {
        auto &com =  view.get(ent);
        // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
        ImGui::PushID(&com);
        if (ImGui::Selectable(com.name.c_str(), selected == ent))
            selected = ent;
        ImGui::PopID();
    }
    ImGui::EndChild();
    if(Array::null != selected && view.contains(selected))
        return view.get(selected);
    else
        return *((Professor*)nullptr);
}




Array::entity Lession::del(Lession& obj)
{
    // is this lession required by any lession?
    auto view = Lession::registery.view();
    for(auto ent: view) {
        auto &less = view.get(ent);
        if(less.require == obj.id)
            return less.require;
    }
    // is related to any professor
    auto pair = Pair::findProfessor(obj);
    if(Array::null != pair)
        return pair;
    // wheter it is any group
    auto group = Group::isInAny(obj);
    if(Array::null != group)
        return group;
    registery.destroy(obj.id);
    return Array::null;
}
void Lession::print(){
    ImGui::Text("#%lu",this->id);
    ImGui::InputText("name##name",this->name.data(),this->name.capacity()-1);

    ImGui::Combo("hours", &this->hours, hours_lable, 3);
    ImGui::SameLine();
    info("Total number of hours per 2 week.");

    ImGui::InputInt("semister", &this->semister, 1, 1);
    ImGui::SameLine();
    info("1~8: Normal\n0: No conflict with Any\n9: Conflict with all lessions on same hour");
    this->semister = max(0,this->semister);
    this->semister = min(9,this->semister);

    ImGui::InputInt("weight", &this->weight, 1, 2);
    ImGui::SameLine();
    info("0~10: Add more periority to place this item first\nNo effect on conflict with other lessions");
    this->weight = max(0, this->weight);
    this->weight = min(10, this->weight);

    ImGui::Text("Requires");
    ImGui::SameLine();
    info("Set a conflict with a lession.");
    ImGui::SameLine();
    if (ImGui::Button("set"))
        ImGui::OpenPopup("Select a lession##2");
    if (ImGui::BeginPopupModal("Select a lession##2", NULL, 0))
    {
        Lession::popup_require(*this);
        ImGui::EndPopup();
    }
    ImGui::Indent(1.0f);{
        this->print_required();
    }ImGui::Unindent(1.0f);

    ImGui::Text("Assigned professor");
    ImGui::SameLine();
    if (ImGui::Button("Assign"))
        ImGui::OpenPopup("Select a professor##1");
    if (ImGui::BeginPopupModal("Select a professor##1", NULL, 0))
    {
        Professor::popup_select(this->id);
        ImGui::EndPopup();
    }
    ImGui::Indent(1.0f);{
        this->print_professor();
    }ImGui::Unindent(1.0f);
}
void Lession::list(Array::entity entity)
{
    auto &com = registery.get(entity);
    ImGui::PushID(&com);
    if(ImGui::Button("X"))
        Pair::deleteByLession(com);
    ImGui::PopID();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Unassign");
    ImGui::SameLine();
    ImGui::Text("%s",com.name.c_str());
}
void Lession::print_required() {
    if(Array::null != this->require)
    {
        const auto& req = registery.get(this->require);
        ImGui::PushID(&req);
        if(ImGui::Button("X"))
            this->require = Array::null;
        ImGui::PopID();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Unassign");
        ImGui::SameLine();
        ImGui::Text("%s",req.name.c_str());
    }else{
        ImGui::Text("None");
    }
}
void Lession::print_professor() const {
    auto prof_id = Pair::findProfessor(*this);
    if(Array::null != prof_id)
    {
        const auto& professor = Professor::registery.get(prof_id);
        ImGui::PushID(&professor);
        if(ImGui::Button("X"))
            Pair::deleteByLession(*this);
        ImGui::PopID();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Unassign");
        ImGui::SameLine();
        ImGui::Text("%s",professor.name.c_str());
    }else{
        ImGui::Text("None");
    }
}
void Lession::popup_add(Array::entity professor)
{
    static Array::entity popup_selected_less = Array::null;
    ImGui::BeginChild("lesson_popup", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()-5), true);
    auto view = registery.view();
    for(auto ent: view)
    {
        auto &less =  view.get(ent);
        ImGui::PushID(&less);
        if (ImGui::Selectable(less.name.c_str(), popup_selected_less == ent))
            popup_selected_less = ent;
        ImGui::PopID();
    }
    ImGui::EndChild();

    bool cc = ImGui::Button("Cancel");
    ImGui::SameLine();
    bool ok = ImGui::Button("OK");
    if(ok && Array::null != popup_selected_less)
        Pair::assign(popup_selected_less,professor);
    if (ok || cc)
        ImGui::CloseCurrentPopup();
}
void Lession::popup_require(Lession &lession) {
    static Array::id_type popup_selected_less = Array::null;
    ImGui::BeginChild("lesson_popup", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()-5), true);
    auto view = registery.view();
    for(Array::entity ent: view) {
        auto &less =  view.get(ent);
        ImGui::PushID(&less);
        if (ImGui::Selectable(less.name.c_str(), popup_selected_less == ent))
            popup_selected_less = ent;
        ImGui::PopID();
    }
    ImGui::EndChild();

    bool cc = ImGui::Button("Cancel");
    ImGui::SameLine();
    bool ok = ImGui::Button("OK");
    if(ok){
        if(Array::null != popup_selected_less && lession.id != popup_selected_less)
            lession.require = popup_selected_less;
    }
    if (ok || cc)
        ImGui::CloseCurrentPopup();
}
Lession& Lession::panel()
{
    static Array::entity selected = Array::null;
    ImGui::BeginChild("leftpane", ImVec2(150, 0), true);
    auto view = registery.view();
    // use forward iterators and get only the components of interest
    for(Array::entity ent: view) {
        auto &com =  view.get(ent);
        // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
        ImGui::PushID(&com);
        if (ImGui::Selectable(com.name.c_str(), selected == ent))
            selected = ent;
        ImGui::PopID();
    }
    ImGui::EndChild();
    if(Array::null != selected && view.contains(selected))
        return view.get(selected);
    else
        return *((Lession*)nullptr);
}






void Group::print() {
    ImGui::Text("#%lu",this->id);
    ImGui::InputText("name##name",this->name.data(),this->name.capacity()-1);
    ImGui::Combo("priority", &this->priority, Group::priorities, 3);
    ImGui::Text("members");
    ImGui::SameLine();
    if (ImGui::Button("add"))
        ImGui::OpenPopup("Select a lession##3");
    if (ImGui::BeginPopupModal("Select a lession##3", NULL, 0))
    {
        this->popup_add();
        ImGui::EndPopup();
    }
    ImGui::Indent(1.0f);{
        this->list();
    }ImGui::Unindent(1.0f);
}
void Group::list() {
    auto begin=this->members.begin();
    auto end=this->members.end();
    for(;begin!=end;begin++)
    {
        const auto member = *begin;
        auto &com = Lession::registery.get(member);
        ImGui::PushID(&com);
        if(ImGui::Button("X")) {
            this->members.erase(begin);
            // causes crash if array has 1 element remainding
            ImGui::PopID();break;
        }
        ImGui::PopID();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Unassign");
        ImGui::SameLine();
        ImGui::Text("%s",com.name.c_str());
    }
}
void Group::popup_add()
{
    static Array::entity popup_selected_less = Array::null;
    ImGui::BeginChild("lesson_popup", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()-5), true);
    auto view = Lession::registery.view();
    for(Array::id_type ent: view) {
        auto &less =  view.get(ent);
        ImGui::PushID(&less);
        if (ImGui::Selectable(less.name.c_str(), popup_selected_less == ent))
            popup_selected_less = ent;
        ImGui::PopID();
    }
    ImGui::EndChild();

    bool cc = ImGui::Button("Cancel");ImGui::SameLine();bool ok = ImGui::Button("OK");
    if(ok){
        if(Array::null != popup_selected_less){
            for(auto &entt:this->members)
                if(popup_selected_less == entt)
                    goto fail;
            this->members.push_back(popup_selected_less);
        }
    }
    fail:
    if (ok || cc)
        ImGui::CloseCurrentPopup();
}
Group& Group::panel()
{
    static Array::entity selected = Array::null;
    ImGui::BeginChild("leftpane", ImVec2(150, 0), true);
    auto view = registery.view();
    // use forward iterators and get only the components of interest
    for(Array::entity ent: view) {
        auto &com =  view.get(ent);
        // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
        ImGui::PushID(&com);
        if (ImGui::Selectable(com.name.c_str(), selected == ent))
            selected = ent;
        ImGui::PopID();
    }
    ImGui::EndChild();
    if(Array::null != selected && view.contains(selected))
        return view.get(selected);
    else
        return *((Group*)nullptr);
}


bool TimeTable::panel() {
    ImGui::BeginChild("TimeTableList", ImVec2(0, -30), true);
    Score total = 0;
    for (int i = 0; i < 50; i += 2) {
        ImGui::Text(lable_name[i / 2]);
        ImGui::Indent(10.0f);
        auto& day_even = this->hours[i];
        auto& day_odd = this->hours[i + 1];
        if(day_even.empty() && day_odd.empty())
            ImGui::Text("Nothing");
        for (const auto& value : day_even) {
            const EnchantLession* c = value.lession;
            const char* day_type;
            if (day_odd.count(value))
                day_type = "both";
            else
                day_type = "even";
            ImGui::Text("%s (%s): %s ,score: %d", c->assigned_professor->name, day_type, c->name, value.score);
            total += value.score;
        }
        for (const auto& value : day_odd) {
            const EnchantLession* c = value.lession;
            if (!day_even.count(value)) {
                ImGui::Text("%s (%s): %s ,score: %d", c->assigned_professor->name, " odd", c->name, value.score);
                total += value.score;
            }
        }
        ImGui::Unindent(10.0f);
    }
    ImGui::EndChild();
    ImGui::Text("total score: %d", total);
    ImGui::SameLine();
    if (ImGui::Button("Save"))
        this->print_result();
    ImGui::SameLine();
    if (ImGui::Button("OK"))
        return true;
    return false;
}
