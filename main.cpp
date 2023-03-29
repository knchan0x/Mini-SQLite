#include <iostream>
#include <string>

struct InputBuffer
{
    std::string buffer;
};

enum class MetaCommandResult {
    SUCCESS,
    UNRECOGNIZED_COMMAND
};

enum class PrepareResult {
    SUCCESS,
    UNRECOGNIZED_STATEMENT
};

enum class StatementType {
    INSERT,
    SELECT
};

struct Statement {
    StatementType type;
};

InputBuffer *new_input_buffer()
{
    InputBuffer *input_buffer = new InputBuffer;
    return input_buffer;
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer) {
        if (input_buffer->buffer.find(".exit") == 0)
        {
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }
        else
        {
            return MetaCommandResult::UNRECOGNIZED_COMMAND;
        }
}

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    if (input_buffer->buffer.find("insert") == 0) {
        statement->type = StatementType::INSERT;
        return PrepareResult::SUCCESS;
    }
    if (input_buffer->buffer.find("select") == 0) {
        statement->type = StatementType::SELECT;
        return PrepareResult::SUCCESS;
    }
    return PrepareResult::UNRECOGNIZED_STATEMENT;
}


void print_prompt()
{
    std::cout << "db > ";
}

void read_input(InputBuffer *input_buffer)
{
    std::getline(std::cin, input_buffer->buffer);

    if (input_buffer->buffer.size() <= 0)
    {
        std::cout << "Error reading input" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void close_input_buffer(InputBuffer *input_buffer)
{
    delete input_buffer;
}

void execute_statement(Statement* statement) {
    switch (statement->type) {
    case StatementType::INSERT:
        std::cout << "This is where we would do an insert." << std::endl;
        break;
    case StatementType::SELECT:
        std::cout << "This is where we would do a select." << std::endl;
        break;
    }
}

int main(int argc, char *argv[])
{
    InputBuffer *input_buffer = new_input_buffer();
    while (true)
    {
        print_prompt();
        read_input(input_buffer);
        if (input_buffer->buffer.find(".") == 0)
        {
            switch (do_meta_command(input_buffer))
            {
            case MetaCommandResult::SUCCESS:
                /* code */
                continue;
            case MetaCommandResult::UNRECOGNIZED_COMMAND:
                std::cout << "Unrecognized command " << input_buffer->buffer << std::endl;
                continue;
            }
        }

        Statement statement;
        switch (prepare_statement(input_buffer, &statement))
        {
        case PrepareResult::SUCCESS:
            /* code */
            break;
        case PrepareResult::UNRECOGNIZED_STATEMENT:
            std::cout << "Unrecognized keyword at start of " << input_buffer->buffer << std::endl;
            continue;
        }

        execute_statement(&statement);
        std::cout << "Executed." << std::endl;
    }
    return 0;
}
