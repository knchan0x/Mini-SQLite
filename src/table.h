#pragma once

#include <iostream>
#include <array>

const uint32_t COLUMN_USERNAME_SIZE = 32;
const uint32_t COLUMN_EMAIL_SIZE = 255;

struct Row
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1]; // cannot use std::array due to sscanf
    /* 1 padding byte for email size 33 */
    char email[COLUMN_EMAIL_SIZE + 1]; // cannot use std::array due to sscanf
    void print();
};

const uint32_t ID_SIZE = sizeof(Row::id);
const uint32_t USERNAME_SIZE = sizeof(Row::username);
const uint32_t EMAIL_SIZE = sizeof(Row::email);

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

void serialize_row(Row *source, Page *destination, uint32_t row_num);
void deserialize_row(Page *source, Row *destination, uint32_t row_num);

struct Table
{
    uint32_t num_rows;
    std::array<Page *, TABLE_MAX_PAGES> pages;

    Table()
    {
        this->num_rows = 0;
        for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
        {
            this->pages[i] = nullptr;
        }
    }

    ~Table()
    {
        for (auto page : this->pages)
        {
            if (page)
            {
                delete[] page;
            }
        }
    }
};

Page *row_slot(Table *table, uint32_t row_num);