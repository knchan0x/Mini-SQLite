#include <string>
#include <sstream>

#include "processor.hpp"

Statement *CommandProcessor::parse_meta_command(InputBuffer *input_buffer)
{
    if (input_buffer->buffer.find(".exit") == 0)
    {
        return new Statement(ParseResult::SUCCESS, StatementType::EXIT);
    }
    else if (input_buffer->buffer.find(".btree") == 0)
    {
        return new Statement(ParseResult::SUCCESS, StatementType::TREE);
    }
    else if (input_buffer->buffer.find(".constant") == 0)
    {
        return new Statement(ParseResult::SUCCESS, StatementType::TREE);
    }
    else
    {
        return new Statement(ParseResult::UNRECOGNIZED_META_COMMAND);
    }
}

const uint32_t INSERT_POSITION_ID = 1;
const uint32_t INSERT_POSITION_USERNAME = 2;
const uint32_t INSERT_POSITION_EMAIL = 3;

Statement *CommandProcessor::parse_insert(InputBuffer *input_buffer)
{
    std::vector<std::string> tokens;
    std::stringstream inputs(input_buffer->buffer);
    std::string token;
    while (std::getline(inputs, token, ' '))
    {
        tokens.push_back(token);
    }

    if (tokens.size() > 4)
    {
        return new Statement(ParseResult::SYNTAX_ERROR);
    }

    uint32_t id = atoi(tokens[INSERT_POSITION_ID].c_str());
    if (id < 0)
    {
        return new Statement(ParseResult::NEGATIVE_ID);
    }
    if (tokens[INSERT_POSITION_USERNAME].size() > COLUMN_USERNAME_SIZE ||
        tokens[INSERT_POSITION_EMAIL].size() > COLUMN_EMAIL_SIZE)
    {
        return new Statement(ParseResult::STRING_TOO_LONG);
    }

    Statement *statement = new Statement(ParseResult::SUCCESS, StatementType::INSERT);
    statement->row_to_insert.id = id;
    std::strcpy(statement->row_to_insert.username, tokens[INSERT_POSITION_USERNAME].c_str());
    std::strcpy(statement->row_to_insert.email, tokens[INSERT_POSITION_EMAIL].c_str());

    return statement;
}

Statement *CommandProcessor::parse_select(InputBuffer *input_buffer)
{
    return new Statement(ParseResult::SUCCESS, StatementType::SELECT);
}

Statement *CommandProcessor::parse_statement(InputBuffer *input_buffer)
{
    if (input_buffer->buffer.find("insert") == 0)
    {
        return this->parse_insert(input_buffer);
    }
    if (input_buffer->buffer.find("select") == 0)
    {
        return this->parse_select(input_buffer);
    }
    return new Statement(ParseResult::UNRECOGNIZED_STATEMENT);
}

Statement *CommandProcessor::parse(InputBuffer *input_buffer)
{
    if (input_buffer->buffer.find(".") == 0)
    {
        return this->parse_meta_command(input_buffer);
    }
    else
    {
        return this->parse_statement(input_buffer);
    }
}