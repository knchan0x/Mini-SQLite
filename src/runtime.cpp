#include <iostream>

#include "db.hpp"
#include "runtime.hpp"
#include "table.hpp"
#include "vm.hpp"

Runtime::Runtime(Database *db) : db(db) {}

void Runtime::print_prompt()
{
    std::cout << "db > ";
}

// read input and store it in input_buffer
// return true if successful, otherwise, return false
bool Runtime::read_input(InputBuffer &input_buffer)
{
    std::getline(std::cin, input_buffer.buffer);

    if (input_buffer.buffer.size() <= 0)
    {
        return false;
    }
    return true;
}

void Runtime::indefinite_loop()
{
    InputBuffer input_buffer;
    CommandProcessor processor;
    VirtualMachine vm(this->db->get_table()); // get default table

    bool flag = true;
    while (flag)
    {
        print_prompt();

        if (!read_input(input_buffer))
        {
            std::cout << "Unable to read input" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        auto [parse_result, parse_statement] = processor.parse(input_buffer);

        switch (parse_result)
        {
        case ParseResult::SUCCESS:
            switch (vm.execute(*parse_statement))
            {
            case ExecuteResult::SUCCESS:
                std::cout << "Executed." << std::endl;
                break;
            case ExecuteResult::DUPLICATE_KEY:
                std::cout << "Error: Duplicate key." << std::endl;
                break;
            case ExecuteResult::EXIT:
                flag = false;
                break;
            }
            break;
        case ParseResult::UNRECOGNIZED_META_COMMAND:
            std::cout << "Unrecognized command " << input_buffer.buffer << std::endl;
            break;
        case ParseResult::NEGATIVE_ID:
            std::cout << "ID must be positive." << std::endl;
            break;
        case ParseResult::STRING_TOO_LONG:
            std::cout << "String is too long." << std::endl;
            break;
        case ParseResult::SYNTAX_ERROR:
            std::cout << "Syntax error. Could not parse statement." << std::endl;
            break;
        case ParseResult::UNRECOGNIZED_STATEMENT:
            std::cout << "Unrecognized keyword at start of " << input_buffer.buffer << std::endl;
            break;
        }

        delete parse_statement;
    }
}
