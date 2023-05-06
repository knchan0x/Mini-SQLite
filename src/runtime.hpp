#pragma once

#include "db.hpp"

// a REPL environment
class Runtime
{
public:
    // functions

    Runtime(Database *db);

    void indefinite_loop();

private:
    // variables

    Database *db;
};