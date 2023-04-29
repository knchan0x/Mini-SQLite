#pragma once

#include "processor.hpp"

struct Cursor
{
    Table *table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates is the cursor locate in a position after the last element

    Cursor(Table *table);

    void move(CursorPosition position);

    Row *value();
    void advance();

    Cursor *find(uint32_t key);
    Cursor *leaf_node_find(uint32_t page_num, uint32_t key);
    Cursor *internal_node_find(uint32_t page_num, uint32_t key);
    void insert(uint32_t key, Row *value);
    void split_and_insert(uint32_t key, Row *value);

private:
    void move_begin();
    void move_end();
};

enum class ExecuteResult
{
    SUCCESS,
    DUPLICATE_KEY,
    EXIT
};

class VirtualMachine
{
private:
    // variables

    Table *table;

    // functions

    ExecuteResult print_tree();
    ExecuteResult print_constants();
    ExecuteResult execute_insert(Statement *statement);
    ExecuteResult execute_select(Statement *statement);

public:
    // functions

    VirtualMachine(Table *table);
    ExecuteResult execute(Statement *statement);
};