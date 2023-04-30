#pragma once

#include "pager.hpp"

class Table
{
private:
    // variables

    uint32_t root_page_num;

public:
    // variables
    Pager *pager;

    // functions
    Table(std::string filename);
    ~Table();

    uint32_t get_root();
    Node *new_root(uint32_t page_num);
};