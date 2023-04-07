#include <iostream>
#include <string>
#include <array>
#include <sstream>
#include <vector>

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
    PREPARE_NEGATIVE_ID,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
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
    }
};

MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table)
{
    if (input_buffer->buffer.find(".exit") == 0)
    {
        delete input_buffer;
        delete table;
        std::exit(EXIT_SUCCESS);
    }
    else
    {
        return MetaCommandResult::UNRECOGNIZED_COMMAND;
    }
}

#define INSERT_POSITION_ID = 1
#define INSERT_POSITION_USERNAME = 2
#define INSERT_POSITION_EMAIL = 3

PrepareResult prepare_insert(InputBuffer *input_buffer, Statement *statement)
{
    statement->type = StatementType::INSERT;

    std::vector<std::string> tokens;
    std::stringstream inputs(input_buffer->buffer);
    std::string token;
    while (std::getline(inputs, token, ' '))
    {
        tokens.push_back(token);
    }

    if (tokens.size() > 4)
    {
        return PrepareResult::PREPARE_SYNTAX_ERROR;
    }

    std::string id_string = tokens[1];
    std::string username = tokens[2];
    std::string email = tokens[3];

    int id = atoi(id_string.c_str());
    if (id < 0)
    {
        return PrepareResult::PREPARE_NEGATIVE_ID;
    }
    if (username.size() > COLUMN_USERNAME_SIZE)
    {
        return PrepareResult::PREPARE_STRING_TOO_LONG;
    }
    if (email.size() > COLUMN_EMAIL_SIZE)
    {
        return PrepareResult::PREPARE_STRING_TOO_LONG;
    }

    statement->row_to_insert.id = id;
    std::strcpy(statement->row_to_insert.username, username.c_str());
    std::strcpy(statement->row_to_insert.email, email.c_str());

    return PrepareResult::SUCCESS;
}

PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement)
{
    if (input_buffer->buffer.find("insert") == 0)
    {
        return prepare_insert(input_buffer, statement);
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
    Cursor *cursor = table_end(table);

    serialize_row(row_to_insert, cursor_value(cursor), table->num_rows % ROWS_PER_PAGE);
    table->num_rows += 1;

    delete cursor;

    return ExecuteResult::EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement *statement, Table *table)
{
    Cursor *cursor = table_start(table);
    Row row;
    uint32_t i = 0;
    while (!cursor->end_of_table)
    {
        deserialize_row(cursor_value(cursor), &row, i % ROWS_PER_PAGE);
        row.print();
        cursor_advance(cursor);
        i++;
    };
    delete cursor;
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
    if (argc < 2)
    {
        std::cout << "Must supply a database filename." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    Table *table = new Table(argv[1]);
    InputBuffer *input_buffer = new InputBuffer;
    while (true)
    {
        print_prompt();
        read_input(input_buffer);
        if (input_buffer->buffer.find(".") == 0)
        {
            switch (do_meta_command(input_buffer, table))
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
        case PrepareResult::PREPARE_NEGATIVE_ID:
            std::cout << "ID must be positive." << std::endl;
            continue;
        case PrepareResult::PREPARE_STRING_TOO_LONG:
            std::cout << "String is too long." << std::endl;
            continue;
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
}