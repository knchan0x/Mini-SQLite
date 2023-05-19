#include <string>
#include <sstream>

#include "processor.hpp"

// meta commend
Statement::Statement(StatementType type) : type(type) {}

// normal statement
Statement::Statement(StatementType type, const Row &row_to_insert) : type(type), row_to_insert(row_to_insert) {}

std::tuple<ParseResult, Statement *> CommandProcessor::parse_meta_command(const InputBuffer &input_buffer)
{
    if (input_buffer.buffer.find(".exit") == 0)
    {
        return std::make_tuple(ParseResult::SUCCESS, new Statement(StatementType::EXIT));
    }
    else if (input_buffer.buffer.find(".btree") == 0)
    {
        return std::make_tuple(ParseResult::SUCCESS, new Statement(StatementType::TREE));
    }
    else if (input_buffer.buffer.find(".constant") == 0)
    {
        return std::make_tuple(ParseResult::SUCCESS, new Statement(StatementType::CONSTANTS));
    }
    else
    {
        return std::make_tuple(ParseResult::UNRECOGNIZED_META_COMMAND, nullptr);
    }
}

const uint32_t INSERT_POSITION_ID = 1;
const uint32_t INSERT_POSITION_USERNAME = 2;
const uint32_t INSERT_POSITION_EMAIL = 3;

std::tuple<ParseResult, Statement *> CommandProcessor::parse_insert(const InputBuffer &input_buffer)
{
    std::vector<std::string> tokens;
    std::stringstream inputs(input_buffer.buffer);
    std::string token;
    while (std::getline(inputs, token, ' '))
    {
        tokens.push_back(token);
    }

    if (tokens.size() != 4)
    {
        return std::make_tuple(ParseResult::SYNTAX_ERROR, nullptr);
    }

    uint32_t id = std::stoi(tokens[INSERT_POSITION_ID].c_str());
    if (id < 0)
    {
        return std::make_tuple(ParseResult::NEGATIVE_ID, nullptr);
    }
    if (tokens[INSERT_POSITION_USERNAME].size() > COLUMN_USERNAME_SIZE ||
        tokens[INSERT_POSITION_EMAIL].size() > COLUMN_EMAIL_SIZE)
    {
        return std::make_tuple(ParseResult::STRING_TOO_LONG, nullptr);
    }

    Statement *statement = new Statement(StatementType::INSERT);
    statement->row_to_insert.id = id;
    std::strcpy(statement->row_to_insert.username, tokens[INSERT_POSITION_USERNAME].c_str());
    std::strcpy(statement->row_to_insert.email, tokens[INSERT_POSITION_EMAIL].c_str());

    return std::make_tuple(ParseResult::SUCCESS, statement);
}

std::tuple<ParseResult, Statement *> CommandProcessor::parse_select(const InputBuffer &input_buffer)
{
    return std::make_tuple(ParseResult::SUCCESS, new Statement(StatementType::SELECT));
}

std::tuple<ParseResult, Statement *> CommandProcessor::parse_statement(const InputBuffer &input_buffer)
{
    if (input_buffer.buffer.find("insert") == 0)
    {
        return this->parse_insert(input_buffer);
    }
    if (input_buffer.buffer.find("select") == 0)
    {
        return this->parse_select(input_buffer);
    }
    return std::make_tuple(ParseResult::UNRECOGNIZED_STATEMENT, nullptr);
}

std::tuple<ParseResult, Statement *> CommandProcessor::parse(const InputBuffer &input_buffer)
{
    if (input_buffer.buffer.find(".") == 0)
    {
        return this->parse_meta_command(input_buffer);
    }
    else
    {
        return this->parse_statement(input_buffer);
    }
}