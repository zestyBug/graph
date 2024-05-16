#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_my.hpp"

#include "Solver.hpp"
#include "uni.hpp"
#include "ui_components.hpp"
#include "uni_ini.hpp"
#include "string_view.hpp"
#include <stdexcept>
#include "advanced_array/advanced_array.hpp"
#include <map>
#include <list>

#include <fstream>

void load_ini(const char * filename) {
    size_t ini_size = 0;
    char* ini_data = (char*)ImFileLoadToMemory(filename, "rb", &ini_size);
    std::list<Professor> professor_list;
    // lession, professor
    std::map<advanced_array::entity, advanced_array::entity> pair_list;
    std::list<Group> group_list;
    std::list<Lession> lession_list;
    // Zero-tolerance, no error reporting, cheap .ini parsing
    if (ini_data && ini_size > 0) {

        char* const buf = ini_data;
        char* const buf_end = buf + ini_size;
        buf_end[0] = 0;

        char entity_type = 0;

        char* line_end = NULL;
        for (char* line = buf; line < buf_end; line = line_end + 1)
        {
            // Skip new lines markers, then find end of the line
            while (*line == '\n' || *line == '\r')
                line++;
            line_end = line;
            while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
                line_end++;
            line_end[0] = 0;
            if (line[0] == ';')
                continue;
            if (line[0] == '[' && line_end > line && line_end[-1] == ']')
            {
                // Parse "[Type][Name]". Note that 'Name' can itself contains [] characters, which is acceptable with the current format and parsing code.
                line_end[-1] = 0;
                const char* name_end = line_end - 1;
                const char* type_start = line + 1;
                char* type_end = (char*)memchr(type_start, (int)']', name_end - type_start);
                const char* name_start = type_end ? (const char*)memchr(type_end + 1, (int)'[', name_end - (type_end + 1)) : NULL;
                if (!type_end || !name_start)
                    continue;
                *type_end = 0; // Overwrite first ']'
                name_start++;  // Skip second '['
                /*
                * use the following data and set
                * type_start,type_end
                * name_start,name_end
                */
                string_view type(type_start, (size_t)(type_end - type_start));
                string_view name(name_start, (size_t)(name_end - name_start));

                if (type.starts_with("professor")) {
                    professor_list.emplace_back(advanced_array::null, name.data());
                    entity_type = 1;
                }
                else if (type.starts_with("lession")) {
                    lession_list.emplace_back(advanced_array::null, name.data());
                    entity_type = 2;
                }
                else if (type.starts_with("group")) {
                    group_list.emplace_back(advanced_array::null, name.data());
                    entity_type = 3;
                }
                else {
                    entity_type = 0;
                }
            }
            else if (entity_type != 0)
            {
                // line, line_end, entry_handler
                string_view line_view(line, (size_t)(line_end - line));
                int p1[25];
                advanced_array::entity p2;
                if (entity_type == 2) {
                    Lession& lession = lession_list.back();
                    if (sscanf(line_view.data(), "hours=%i", &p1[0]) == 1) {
                        lession.hours = p1[0];
                    }
                    else if (sscanf(line_view.data(), "weight=%i", &p1[0]) == 1) {
                        lession.weight = p1[0];
                    }
                    else if (sscanf(line_view.data(), "require=%lli", &p2) == 1) {
                        lession.require = p2;
                    }
                    else if (sscanf(line_view.data(), "semister=%i", &p1[0]) == 1) {
                        lession.semister = p1[0];
                    }
                    else if (sscanf(line_view.data(), "assign=%llu", &p2) == 1) {
                        pair_list.emplace(lession.id, p2);
                    }
                    else if (sscanf(line_view.data(), "id=%llu", &p2) == 1) {
                        lession.id = p2;
                    }
                }
                else if (entity_type == 1) {
                    Professor& prof = professor_list.back();
                    if (sscanf(line_view.data(),
                        "hours=%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,",
                        p1 + 0, p1 + 1, p1 + 2, p1 + 3, p1 + 4,
                        p1 + 5, p1 + 6, p1 + 7, p1 + 8, p1 + 9,
                        p1 + 10, p1 + 11, p1 + 12, p1 + 13, p1 + 14,
                        p1 + 15, p1 + 16, p1 + 17, p1 + 18, p1 + 19,
                        p1 + 20, p1 + 21, p1 + 22, p1 + 23, p1 + 24
                    ) == 25) {
                        for (size_t i = 0; i < 25; i++)
                        {
                            prof.hours[i] = p1[i];
                        }
                    }
                    else if (sscanf(line_view.data(), "id=%llu", &p2) == 1) {
                        prof.id = p2;
                    }
                }
                else if (entity_type == 3) {
                    Group& group = group_list.back();
                    if (sscanf(line_view.data(), "periority=%i", &p1[0]) == 1) {
                        group.priority = p1[0];
                    }
                    else if (sscanf(line_view.data(), "add %llu", &p2) == 1) {
                        group.members.emplace_back(p2);
                    }
                }
            }
        }
    }
    // previous id to current id
    std::map<advanced_array::entity, advanced_array::entity> professor_proxy;
    // previous id to current id
    std::map<advanced_array::entity, advanced_array::entity> lession_proxy;
    for (const Professor& value : professor_list) {
        advanced_array::entity id = Professor::registery.create();
        Professor& prof = Professor::registery.emplace(id, id, value.name);
        memcpy(prof.hours, value.hours, sizeof(prof.hours));
        professor_proxy.emplace(value.id, id);
    }
    for (const Lession& value : lession_list) {
        advanced_array::entity id = Lession::registery.create();
        Lession& less = Lession::registery.emplace(id, id, value.name);
        less.hours = value.hours;
        less.semister = value.semister;
        less.weight = value.weight;
        less.require = value.require;
        lession_proxy.emplace(value.id, id);
    }

    for (const auto& value : lession_proxy) {
        Lession& less = Lession::registery.get(value.second);
        if (advanced_array::null != less.require) {
            const auto result = lession_proxy.find(less.require);
            if (result != lession_proxy.end()) {
                less.require = result->second;
            }
        }
    }
    for (const auto& value : pair_list) {
        const auto result_lession = lession_proxy.find(value.first);
        const auto result_professor = professor_proxy.find(value.second);
        if (result_lession != lession_proxy.end() && result_professor != professor_proxy.end())
        {
            Pair::create(result_lession->second, result_professor->second);
        }
    }
    for (const Group& value : group_list) {
        advanced_array::entity id = Group::registery.create();
        Group& less = Group::registery.emplace(id, id, value.name.c_str());
        less.priority = value.priority;
        less.members.reserve(value.members.size());
        for (const auto& member : value.members) {
            const auto result_lession = lession_proxy.find(member);
            if (result_lession != lession_proxy.end())
            {
                less.members.emplace_back(result_lession->second);
            }
        }
    }
    ImGui::MemFree(ini_data);
}


void save_ini(const char* filename) {
    try {
        std::ofstream out{ filename ,std::ios_base::out | std::ios_base::binary | std::ios_base::trunc };
        for (const auto& entity : Group::registery.view()) {
            const Group& group = Group::registery.get(entity);
            out << "[group][" << group.name << "]\nid=" << group.id
                << ";\npriority=" << group.priority << ";\n";

            for (const auto& gmember : group.members)
                out << "add " << gmember << ";\n";
        }
        for (const auto& entity : Lession::registery.view()) {
            const Lession& lession = Lession::registery.get(entity);
            out << "[lession][" << lession.name << "]\nid=" << lession.id
                << ";\nsemister=" << lession.semister
                << ";\nweight=" << lession.weight
                << ";\nhour=" << lession.hours
                << ";\n";
            const auto prof = Pair::findProfessor(lession);
            if(advanced_array::null != prof)
                out << "assign=" << prof << ";\n";
            if (advanced_array::null != lession.require)
                out << "require=" << lession.require << ";\n";
        }
        for (const auto& entity : Professor::registery.view()) {
            const Professor& professor = Professor::registery.get(entity);
            out << "[professor][" << professor.name << "]\nid=" << professor.id << ";\nhours=";
            for (const auto& hour : professor.hours)
                out << (int)hour << ',';
            out << ";\n";
        }
        out.close();
    }
    catch (const std::exception& e) {
        terminal.append("save_ini failed");
    }
}