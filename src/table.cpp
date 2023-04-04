#include <iostream>
#include <string>

#include "table.h"

void serialize_row(Row *source, Page *destination, uint32_t row_num)
{
    destination->rows[row_num] = *source;
};

void deserialize_row(Page *source, Row *destination, uint32_t row_num)
{
    *destination = source->rows[row_num];
};

void Row::print()
{
    std::cout << this->id
              << " "
              << std::string(std::begin(this->username), std::end(this->username))
              << " "
              << std::string(std::begin(this->email), std::end(this->email))
              << std::endl;
}

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