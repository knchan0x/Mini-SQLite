#pragma once

#include "processor.hpp"

enum class CursorPosition
{
    BEGIN,
    END
};

class Cursor
{
private:
    // functions
    
    void move_begin();

    Cursor *leaf_node_find(uint32_t page_num, uint32_t key);
    Cursor *internal_node_find(uint32_t page_num, uint32_t key);

    void split_and_insert(uint32_t key, Row *value);
    void insert_internal_node(uint32_t parent_page_num, uint32_t child_page_num);

public:
    // variables

    Table *table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates is the cursor locate in a position after the last element

    // functions

    Cursor(Table *table);

    void insert(uint32_t key, Row *value);
    Cursor *find(uint32_t key);

    void advance();
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