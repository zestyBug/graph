#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "./uni.hpp"



extern Array::entity selected_prof;
extern Array::entity selected_less;
extern std::string terminal;
extern const float terminal_size;

void professor_tab();

void lession_tab();

void group_tab();

void rel_tab();

void setting_tab();

void terminal_tab();

void forbid_tab();

void relation_tab();


#endif // UI_COMPONENTS_H
