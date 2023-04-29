#pragma once

#include "table.hpp"

struct InputBuffer
{
    std::string buffer;
};

enum class ParseResult
{
    SUCCESS,
    UNRECOGNIZED_META_COMMAND,
    UNRECOGNIZED_STATEMENT,
    SYNTAX_ERROR,
    NEGATIVE_ID,
    STRING_TOO_LONG,
};

enum class StatementType
{
    EXIT,
    TREE,
    CONSTANTS,
    INSERT,
    SELECT
};

struct Statement
{
    ParseResult result;
    StatementType type;
    Row row_to_insert;

    // parse failure
    Statement(ParseResult result)
        : result(result)
    {
    }

    // meta commend
    Statement(ParseResult result, StatementType type)
        : result(result), type(type)
    {
    }

    // normal statement
    Statement(ParseResult result, StatementType type, Row row_to_insert)
        : result(result), type(type), row_to_insert(row_to_insert)
    {
    }
};

class CommandProcessor
{
private:
    // functions

    Statement *parse_meta_command(InputBuffer *input_buffer);
    Statement *parse_statement(InputBuffer *input_buffer);
    Statement *parse_insert(InputBuffer *input_buffer);
    Statement *parse_select(InputBuffer *input_buffer);

public:
    // functions

    // build and return an new statement
    Statement *parse(InputBuffer *InputBuffer);
};