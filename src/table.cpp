#include <iostream>
#include <string>

#include "table.h"

void Row::print()
{
    std::cout << this->id
              << " "
              << std::string(std::begin(this->username), std::end(this->username))
              << " "
              << std::string(std::begin(this->email), std::end(this->email))
              << std::endl;
}

Pager::Pager(std::string filename)
{
    this->filename = filename;
    std::ifstream file = std::ifstream(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        // create file if it is not exit
        std::ofstream new_file = std::ofstream(filename, std::ofstream::trunc | std::fstream::binary);
        new_file.close();
        file = std::ifstream(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            std::cout << "Unable to create file: " << filename << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    else
    {
        this->file_length = file.tellg();
        file.close();
    };

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        this->pages[i] = nullptr;
    }
}

Pager::~Pager()
{
    for (auto page : this->pages)
    {
        if (page)
        {
            delete page;
        }
    }
}

void Pager::flush(uint32_t page_num, uint32_t size)
{
    if (this->pages[page_num] == NULL)
    {
        std::cout << "Tried to flush null page." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::ofstream file = std::ofstream(filename, std::ios::trunc | std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "Fail to open file: " << this->filename << std::endl;
        std::exit(EXIT_FAILURE);
    };

    file.seekp(page_num * PAGE_SIZE, std::ios::beg);
    file.write(this->serialize(this->pages[page_num], size), size);

    if (file.bad())
    {
        std::cout << "Read/writing error on i/o operation occur in writing file: " << this->filename << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (file.fail())
    {
        std::cout << "Logical error on i/o operation occur in writing file: " << this->filename << std::endl;
        std::exit(EXIT_FAILURE);
    }

    file.close();
}

Page *Pager::get_page(uint32_t page_num)
{
    if (page_num > TABLE_MAX_PAGES)
    {
        std::cout << "Tried to fetch page number out of bounds."
                  << page_num << " > " << TABLE_MAX_PAGES
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (this->pages[page_num] == nullptr)
    {
        // Cache miss. Allocate memory and load from file.
        uint32_t num_pages = this->file_length / PAGE_SIZE;

        // We might save a partial page at the end of the file
        uint32_t read_size = PAGE_SIZE;
        if (this->file_length % PAGE_SIZE)
        {
            num_pages += 1;
            read_size = this->file_length % PAGE_SIZE;
        }

        char *page_data = new char[PAGE_SIZE];
        if (page_num <= num_pages)
        {
            std::ifstream file = std::ifstream(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open())
            {
                std::cout << "Fail to open file: " << this->filename << std::endl;
                std::exit(EXIT_FAILURE);
            };
            file.seekg(page_num * PAGE_SIZE, std::ios::beg);
            file.read(page_data, read_size);
            if (sizeof(page_data) == 0)
            {
                std::cout << "Error reading file: " << this->filename << std::endl;
                std::exit(EXIT_FAILURE);
            }
            file.close();
        }

        this->pages[page_num] = this->deserialize(page_data, read_size);
    }

    return this->pages[page_num];
}

char *Pager::serialize(Page *page, uint32_t size)
{
    char *page_data = new char[size];
    char *pos = page_data;

    for (uint32_t i = 0; i < size / ROW_SIZE; i++)
    {
        memcpy(pos + ID_OFFSET, &(page->rows[i].id), ID_SIZE);
        memcpy(pos + USERNAME_OFFSET, &(page->rows[i].username), USERNAME_SIZE);
        memcpy(pos + EMAIL_OFFSET, &(page->rows[i].email), EMAIL_SIZE);
        pos += ROW_SIZE;
    }
    return page_data;
}

Page *Pager::deserialize(char *page_data, uint32_t size)
{
    Page *page = new Page;
    uint32_t row_nums = size / ROW_SIZE;
    for (uint32_t i = 0; i < row_nums; i++)
    {
        uint32_t pos = i * ROW_SIZE;
        page->rows[i].id = page_data[pos];
        memcpy(&(page->rows[i].username), &page_data[pos] + USERNAME_OFFSET, USERNAME_SIZE);
        memcpy(&(page->rows[i].email), &page_data[pos] + EMAIL_OFFSET, EMAIL_SIZE);
    }
    return page;
}

void serialize_row(Row *source, Page *destination, uint32_t row_num)
{
    destination->rows[row_num] = *source;
};

void deserialize_row(Page *source, Row *destination, uint32_t row_num)
{
    *destination = source->rows[row_num];
};

Table::Table(std::string filename)
{
    Pager *pager = new Pager(filename);

    this->num_rows = pager->file_length / ROW_SIZE;
    this->pager = pager;
}

Table::~Table()
{
    uint32_t num_full_pages = this->num_rows / ROWS_PER_PAGE;
    for (uint32_t i = 0; i < num_full_pages; i++)
    {
        if (pager->pages[i] == NULL)
        {
            continue;
        }
        pager->flush(i, PAGE_SIZE);
    }

    // There may be a partial page to write to the end of the file
    // This should not be needed after we switch to a B-tree
    uint32_t num_additional_rows = this->num_rows % ROWS_PER_PAGE;
    if (num_additional_rows > 0)
    {
        uint32_t page_num = num_full_pages;
        if (pager->pages[page_num] != NULL)
        {
            pager->flush(page_num, num_additional_rows * ROW_SIZE);
        }
    }

    delete pager;
}

Cursor *table_start(Table *table)
{
    Cursor *cursor = new Cursor;
    cursor->table = table;
    cursor->row_num = 0;
    cursor->end_of_table = (table->num_rows == 0);
    return cursor;
}

Cursor *table_end(Table *table)
{
    Cursor *cursor = new Cursor;
    cursor->table = table;
    cursor->row_num = table->num_rows;
    cursor->end_of_table = true;
    return cursor;
}

Page *cursor_value(Cursor *cursor)
{
    return cursor->table->pager->get_page(cursor->row_num / ROWS_PER_PAGE);
}

void cursor_advance(Cursor *cursor)
{
    cursor->row_num += 1;
    if (cursor->row_num >= cursor->table->num_rows)
    {
        cursor->end_of_table = true;
    }
}