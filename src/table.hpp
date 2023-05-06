#pragma once

#include "pager.hpp"

class Table
{
public:
    // variables
    Pager *pager;

    // functions
    Table(const std::string &filename);
    ~Table();

    uint32_t get_root();
    Node &new_root(uint32_t page_num);

private:
    // variables

    uint32_t root_page_num;
};