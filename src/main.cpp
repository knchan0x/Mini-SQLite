#include <iostream>
#include <string>
#include <array>

#include "table.h"

struct InputBuffer
{
    std::string buffer;
};

enum class ExecuteResult
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
};

enum class MetaCommandResult
{
    SUCCESS,
    UNRECOGNIZED_COMMAND
};

enum class PrepareResult
{
    SUCCESS,
    PREPARE_SYNTAX_ERROR,
    UNRECOGNIZED_STATEMENT
};

enum class StatementType
{
    INSERT,
    SELECT
};

struct Statement
{
    StatementType type;
    Row row_to_insert;

    Statement()
    {
        this->row_to_insert.id = 0;
        std::fill(this->row_to_insert.username, this->row_to_insert.username + USERNAME_SIZE, 0);
        std::fill(this->row_to_insert.email, this->row_to_insert.email + EMAIL_SIZE, 0);
    }
};

void close_input_buffer(InputBuffer *input_buffer)
{
    delete input_buffer;
}

MetaCommandResult do_meta_command(InputBuffer *input_buffer)
{
    if (input_buffer->buffer.find(".exit") == 0)
    {
        close_input_buffer(input_buffer);
        std::exit(EXIT_SUCCESS);
    }
    else
    {
        return MetaCommandResult::UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement)
{
    if (input_buffer->buffer.find("insert") == 0)
    {
        statement->type = StatementType::INSERT;

        int args_assigned = std::sscanf(
            input_buffer->buffer.c_str(), "insert %d %s %s", &(statement->row_to_insert.id),
            statement->row_to_insert.username, statement->row_to_insert.email);

        if (args_assigned < 3)
        {
            return PrepareResult::PREPARE_SYNTAX_ERROR;
        }

        return PrepareResult::SUCCESS;
    }
    if (input_buffer->buffer.find("select") == 0)
    {
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

ExecuteResult execute_insert(Statement *statement, Table *table)
{
    if (table->num_rows >= TABLE_MAX_ROWS)
    {
        return ExecuteResult::EXECUTE_TABLE_FULL;
    }

    Row *row_to_insert = &(statement->row_to_insert);

    serialize_row(row_to_insert, row_slot(table, table->num_rows), table->num_rows % ROWS_PER_PAGE);
    table->num_rows += 1;

    return ExecuteResult::EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement *statement, Table *table)
{
    Row row;
    for (uint32_t i = 0; i < table->num_rows; i++)
    {
        deserialize_row(row_slot(table, i), &row, i % ROWS_PER_PAGE);
        row.print();
    }
    return ExecuteResult::EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table)
{
    switch (statement->type)
    {
    case StatementType::INSERT:
        return execute_insert(statement, table);
    case StatementType::SELECT:
        return execute_select(statement, table);
    }
}

int main(int argc, char *argv[])
{
    Table *table = new Table;
    InputBuffer *input_buffer = new InputBuffer;
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

        Statement *statement = new Statement;
        switch (prepare_statement(input_buffer, statement))
        {
        case PrepareResult::SUCCESS:
            break;
        case (PrepareResult::PREPARE_SYNTAX_ERROR):
            std::cout << "Syntax error. Could not parse statement." << std::endl;
            continue;
        case PrepareResult::UNRECOGNIZED_STATEMENT:
            std::cout << "Unrecognized keyword at start of " << input_buffer->buffer << std::endl;
            continue;
        }

        switch (execute_statement(statement, table))
        {
        case (ExecuteResult::EXECUTE_SUCCESS):
            std::cout << "Executed." << std::endl;
            break;
        case (ExecuteResult::EXECUTE_TABLE_FULL):
            std::cout << "Error: Table full." << std::endl;
            break;
        }
    }

    delete table; // TODO
}