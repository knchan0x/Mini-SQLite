#pragma once

#include <fstream>

const uint32_t COLUMN_USERNAME_SIZE = 32;
const uint32_t COLUMN_EMAIL_SIZE = 255;

struct Row
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
    void print();
};

const uint32_t ID_SIZE = sizeof(Row::id);
/* 1 padding bit for singular bit size */
const uint32_t USERNAME_SIZE = sizeof(Row::username) % 2 ? sizeof(Row::username) + 1 : sizeof(Row::username);
const uint32_t EMAIL_SIZE = sizeof(Row::email) % 2 ? sizeof(Row::email) + 1 : sizeof(Row::email);

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t TABLE_MAX_PAGES = 100;
const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

struct Page
{
    std::array<Row, ROWS_PER_PAGE> rows; // no overhead
};

struct Pager
{
    std::string filename;
    std::streampos file_length;
    std::array<Page *, TABLE_MAX_PAGES> pages;

    Pager(std::string filename);
    ~Pager();

    Page *get_page(uint32_t page_num);
    void flush(uint32_t page_num, uint32_t size);
    char *serialize(Page *page, uint32_t size);
    Page *deserialize(char *page_data, uint32_t size);
};

void serialize_row(Row *source, Page *destination, uint32_t row_num);
void deserialize_row(Page *source, Row *destination, uint32_t row_num);

struct Table
{
    uint32_t num_rows;
    Pager *pager;

    Table(std::string filename);
    ~Table();
};

Page *row_slot(Table *table, uint32_t row_num);