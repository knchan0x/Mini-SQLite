#include <iostream>

#include "db.hpp"
#include "runtime.hpp"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Must supply a database filename." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Database db = Database(argv[1]);
    Runtime runtime = Runtime(&db);
    runtime.indefinite_loop();
}