#include <iostream>

#include "db.hpp"

const char *UNKNOWN_TABLE_NAME = "Default_Table";

Database::Database(const std::string &filename)
{
    Table *table = new Table(filename);
    tables[UNKNOWN_TABLE_NAME] = table;
}

Database::~Database()
{
    std::cout << "Closing database, please wait..." << std::endl;
    delete this->tables[UNKNOWN_TABLE_NAME];
    std::cout << "Database closed." << std::endl;
}

Table *Database::get_table(const std::string &table_name)
{
    return tables[table_name];
}