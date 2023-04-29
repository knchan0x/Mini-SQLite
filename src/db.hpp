#pragma once

#include <map>

#include "table.hpp"

// Only one table is supported, essentially a swapper of Table
class Database
{
private:
    // Currently, only support one table which is "Default_Table"
    std::unordered_map<std::string, Table *> tables;

public:
    // functions

    Database(std::string filename);
    ~Database();

    Table *get_table(std::string table_name);
};