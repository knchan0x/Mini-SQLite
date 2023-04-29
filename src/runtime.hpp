#pragma once

#include "db.hpp"

// a REPL environment
class Runtime
{
private:
    // variables

    Database *db;

public:
    // functions
    
    Runtime(Database *db);

    void indefinite_loop();
};