#include <iostream>
#include <string>
#include <array>

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

const uint32_t COLUMN_USERNAME_SIZE = 32;
const uint32_t COLUMN_EMAIL_SIZE = 255;
struct Row
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE]; // cannot use std::array due to sscanf
    char email[COLUMN_EMAIL_SIZE];       // cannot use std::array due to sscanf
    /* 1 padding byte for email size 255 */
};

Row *new_row()
{
    return new Row;
}

const uint32_t ID_SIZE = sizeof(Row::id);
const uint32_t USERNAME_SIZE = sizeof(Row::username);
const uint32_t EMAIL_SIZE = sizeof(Row::email);

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

struct Statement
{
    StatementType type;
    Row row_to_insert;
};

Statement *new_statement()
{
    Statement *statement = new Statement;
    statement->row_to_insert.id = 0;
    std::fill(statement->row_to_insert.username, statement->row_to_insert.username + USERNAME_SIZE, 0);
    std::fill(statement->row_to_insert.email, statement->row_to_insert.email + EMAIL_SIZE, 0);
    return statement;
}

InputBuffer *new_input_buffer()
{
    InputBuffer *input_buffer = new InputBuffer;
    return input_buffer;
}

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

const uint32_t TABLE_MAX_PAGES = 100;
const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

struct Page
{
    std::array<Row, ROWS_PER_PAGE> rows; // no overhead
};

void serialize_row(Row *source, Page *destination, uint32_t row_num)
{
    destination->rows[row_num] = *source;
};

void deserialize_row(Page *source, Row *destination, uint32_t row_num)
{
    *destination = source->rows[row_num];
};

struct Table
{
    uint32_t num_rows;
    std::array<Page *, TABLE_MAX_PAGES> pages;
};

Page *row_slot(Table *table, uint32_t row_num)
{
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    Page *page = table->pages[page_num];
    if (page == nullptr)
    {
        // Allocate memory only when we try to access page
        table->pages[page_num] = new Page;
        page = table->pages[page_num];
    }
    return page;
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

void print_row(Row *row)
{
    std::cout << row->id
              << " "
              << std::string(std::begin(row->username), std::end(row->username))
              << " "
              << std::string(std::begin(row->email), std::end(row->email))
              << std::endl;
}

ExecuteResult execute_select(Statement *statement, Table *table)
{
    Row row;
    for (uint32_t i = 0; i < table->num_rows; i++)
    {
        deserialize_row(row_slot(table, i), &row, i % ROWS_PER_PAGE);
        print_row(&row);
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

Table *new_table()
{
    Table *table = new Table;
    table->num_rows = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        table->pages[i] = nullptr;
    }
    return table;
}

void delete_table(Table *table)
{
    for (auto page : table->pages)
    {
        if (page)
        {
            delete[] page;
        }
    }
    delete table;
}

int main(int argc, char *argv[])
{
    Table *table = new_table();
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

        Statement *statement = new_statement();
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
}