#pragma once

#include <tuple>

#include "table.hpp"

struct InputBuffer
{
    std::string buffer;

    InputBuffer();

    InputBuffer(const InputBuffer &) = delete;
    InputBuffer &operator=(const InputBuffer &) = delete;
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
    StatementType type;
    Row row_to_insert;

    explicit Statement(StatementType type);                  // meta commend
    Statement(StatementType type, const Row &row_to_insert); // normal statement

    Statement(const Statement &) = delete;
    Statement &operator=(const Statement &) = delete;
};

class CommandProcessor
{
public:
    // functions

    // build and return an new statement
    std::tuple<ParseResult, Statement *> parse(const InputBuffer &input_buffer);

private:
    // functions

    std::tuple<ParseResult, Statement *> parse_meta_command(const InputBuffer &input_buffer);
    std::tuple<ParseResult, Statement *> parse_statement(const InputBuffer &input_buffer);
    std::tuple<ParseResult, Statement *> parse_insert(const InputBuffer &input_buffer);
    std::tuple<ParseResult, Statement *> parse_select(const InputBuffer &input_buffer);
};