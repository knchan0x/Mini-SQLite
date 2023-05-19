#pragma once

#include <map>

#include "table.hpp"

// Only one table is supported, essentially a swapper of Table
class Database
{
public:
    // functions

    explicit Database(const std::string &filename);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    // Currently, only one table is support which is "Default_Table"
    Table *get_table(const std::string &table_name = "Default_Table");

private:
    std::unordered_map<std::string, Table *> tables;
};