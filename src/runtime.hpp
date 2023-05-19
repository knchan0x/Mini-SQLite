#pragma once

#include "db.hpp"
#include "processor.hpp"

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

    // functions

    void print_prompt();
    bool read_input(InputBuffer& input_buffer);
};