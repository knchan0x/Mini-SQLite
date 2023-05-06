#pragma once

#include <tuple>

#include "processor.hpp"

enum class CursorPosition
{
    BEGIN,
    END
};

class Cursor
{
public:
    // variables

    Table *table;

    // functions

    Cursor(Table *table);

    uint32_t get_page_num();
    uint32_t get_cell_num();
    bool is_end_of_table();

    void insert(uint32_t key, const Row &value);
    void find(uint32_t key);
    void advance();

private:
    // variables

    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates is the cursor locate in a position after the last element

    // functions

    void move_begin();

    void leaf_node_find(uint32_t page_num, uint32_t key);
    void internal_node_find(uint32_t page_num, uint32_t key);

    void split_and_insert(uint32_t key, const Row &value);
    void insert_internal_node(uint32_t parent_page_num, uint32_t child_page_num);
};

enum class ExecuteResult
{
    SUCCESS,
    DUPLICATE_KEY,
    EXIT
};

class VirtualMachine
{
public:
    // functions

    VirtualMachine(Table *table);
    ExecuteResult execute(const Statement &statement);

private:
    // variables

    Table *table;

    // functions

    ExecuteResult print_tree();
    ExecuteResult print_constants();
    ExecuteResult execute_insert(const Statement &statement);
    ExecuteResult execute_select(const Statement &statement);
};