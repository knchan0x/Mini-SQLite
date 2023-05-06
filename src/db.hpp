#pragma once

#include <map>

#include "table.hpp"

// Only one table is supported, essentially a swapper of Table
class Database
{
public:
    // functions

    Database(std::string filename);
    ~Database();

    // Currently, only one table is support which is "Default_Table"
    Table *get_table(std::string table_name);

private:
    std::unordered_map<std::string, Table *> tables;
};