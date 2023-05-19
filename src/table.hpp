#pragma once

#include "pager.hpp"

class Table
{
public:
    // variables

    Pager *pager;

    // functions

    explicit Table(const std::string &filename);
    ~Table();

    Table(const Table &) = delete;
    Table &operator=(const Table &) = delete;

    uint32_t get_root();
    Node &new_root(uint32_t page_num);

private:
    // variables

    uint32_t root_page_num;
};