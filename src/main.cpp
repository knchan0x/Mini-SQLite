#include <iostream>
#include <cstdlib>

#include "db.hpp"
#include "runtime.hpp"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Must supply a database filename. Usage: " << argv[0] << " <database_filename>" << std::endl;
        return EXIT_FAILURE;
    }

    Database db(argv[1]);
    Runtime runtime(&db);
    
    try
    {
        runtime.indefinite_loop();
        return EXIT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}