#include <iostream>
#include <string>

#include "table.hpp"

void Row::print()
{
    std::cout << this->id
              << " "
              << std::string(std::begin(this->username), std::end(this->username))
              << " "
              << std::string(std::begin(this->email), std::end(this->email))
              << std::endl;
}

Cell::Cell(uint32_t *key, Row *value)
    : key(key), value(value)
{
}

uint32_t Cell::get_key()
{
    return *this->key;
}

void Cell::set_key(uint32_t key)
{
    *this->key = key;
}

Row *Cell::get_value()
{
    return this->value;
}

void Cell::set_value(Row *row)
{
    *this->value = *row; // copy to
}

Cell *Cell::get_address()
{
    return (Cell *)this->key;
}

Node::Node(NodeType *nodeType, bool *isRoot, Node *parent)
    : nodeType(nodeType), isRoot(isRoot), parent(parent)
{
}

NodeType Node::get_node_type()
{
    return *this->nodeType;
}

void Node::set_node_type(NodeType nodeType)
{
    *this->nodeType = nodeType;
}

LeafNode::LeafNode(NodeType *nodeType, bool *isRoot, Node *parent, uint32_t *num_cells)
    : Node(nodeType, isRoot, parent), num_cells(num_cells)
{
}

uint32_t LeafNode::get_num_cells()
{
    return *num_cells;
}

void LeafNode::set_num_cells(uint32_t num_cells)
{
    *this->num_cells = num_cells;
}

Cell *LeafNode::get_cell(uint32_t cell_num)
{
    return this->cells[cell_num];
}

void LeafNode::set_cell(uint32_t cell_num, Cell *cell)
{
    this->cells[cell_num] = cell;
}

void LeafNode::print()
{
    uint32_t num_cells = *this->num_cells;
    std::cout << "leaf (size " << num_cells << ")" << std::endl;
    for (uint32_t i = 0; i < num_cells; i++)
    {
        uint32_t key = this->cells[i]->get_key();
        std::cout << "  - " << i << " : " << key << std::endl;
    }
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

    this->file_length = file.tellg();
    file.close();

    this->num_pages = file_length / PAGE_SIZE;
    if (file_length % PAGE_SIZE != 0)
    {
        std::cout << "File Corrupted. Db file contains partial page." << std::endl;
        std::exit(EXIT_FAILURE);
    }

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
    for (auto data : this->page_data)
    {
        if (data)
        {
            delete data;
        }
    }
}

void Pager::flush(uint32_t page_num)
{
    if (this->pages[page_num] == nullptr)
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
    file.write(this->page_data[page_num], PAGE_SIZE);

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

LeafNode *Pager::get_page(uint32_t page_num)
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
        uint32_t num_pages = this->num_pages;

        char *data = new char[PAGE_SIZE];
        if (page_num <= num_pages)
        {
            std::ifstream file = std::ifstream(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open())
            {
                std::cout << "Fail to open file: " << this->filename << std::endl;
                std::exit(EXIT_FAILURE);
            };
            file.seekg(page_num * PAGE_SIZE, std::ios::beg);
            file.read(data, PAGE_SIZE);
            if (!file.eof() && file.fail())
            {
                std::cout << "Error reading file: " << this->filename << std::endl;
                file.close();
                std::exit(EXIT_FAILURE);
            }
            file.close();
        }

        this->page_data[page_num] = data;

        if (page_num >= this->num_pages)
        {
            this->num_pages = page_num + 1;
        }
    }

    this->pages[page_num] = this->deserialize(this->page_data[page_num]);
    return this->pages[page_num];
}

LeafNode *Pager::deserialize(char *page_data)
{

    // page_data is a char[]
    // all elements set to 0 by default
    // LeafNode constructed by pointer to
    // the array, so the default value
    // of those elements are all set to 0
    // i.e.
    // nodeType = NodeType::Leaf (=0)
    // isRoot = false (=0)
    // parent = 0x000000000
    // num_cells = 0
    LeafNode *node = new LeafNode(
        (NodeType *)(&page_data[NODE_TYPE_OFFSET]),
        (bool *)(&page_data[IS_ROOT_OFFSET]),
        (Node *)(&page_data[PARENT_POINTER_OFFSET]),
        (uint32_t *)(&page_data[LEAF_NODE_NUM_CELLS_OFFSET]));

    uint32_t body_start = LEAF_NODE_HEADER_SIZE;
    for (uint32_t i = 0; i < LEAF_NODE_MAX_CELLS; i++)
    {
        uint32_t cell_start = body_start + i * LEAF_NODE_CELL_SIZE;
        Cell *cell = new Cell(
            (uint32_t *)(&page_data[cell_start + LEAF_NODE_KEY_OFFSET]),
            (Row *)(&page_data[cell_start + LEAF_NODE_VALUE_OFFSET]));
        node->set_cell(i, cell);
    }
    return node;
}

Table::Table(std::string filename)
{
    this->root_page_num = 0;
    this->pager = new Pager(filename);

    if (this->pager->num_pages == 0)
    {
        // New database file. Initialize page 0 as leaf node.
        LeafNode *root_node = this->pager->get_page(0);
    }
}

Table::~Table()
{
    for (uint32_t i = 0; i < pager->num_pages; i++)
    {
        if (pager->get_page(i) == nullptr)
        {
            continue;
        }
        pager->flush(i);
    }
    delete pager;
}

uint32_t Table::get_root_page_num()
{
    return this->root_page_num;
}

Row *Cursor::value()
{
    LeafNode *node = this->table->pager->get_page(this->page_num);
    return node->get_cell(this->cell_num)->get_value();
}

void Cursor::advance()
{
    LeafNode *node = this->table->pager->get_page(this->page_num);
    this->cell_num += 1;
    // TODO: Next page
    if (this->cell_num >= node->get_num_cells())
    {
        this->end_of_table = true;
    }
}

void Cursor::insert(uint32_t key, Row *value)
{
    LeafNode *node = this->table->pager->get_page(this->page_num);

    uint32_t num_cells = node->get_num_cells();
    if (num_cells >= LEAF_NODE_MAX_CELLS)
    {
        // Node full
        std::cout << "Need to implement splitting a leaf node." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (this->cell_num < num_cells)
    {
        // Make room for new cell
        for (uint32_t i = num_cells; i > this->cell_num; i--)
        {
            memcpy(node->get_cell(i)->get_address(), node->get_cell(i - 1)->get_address(), LEAF_NODE_CELL_SIZE);
        }
    }

    node->set_num_cells(num_cells + 1);
    Cell *cell = node->get_cell(this->cell_num); // TODO: operator reload []
    cell->set_key(key);
    cell->set_value(value);
}

Cursor::Cursor(Table *table, CursorPosition position)
    : table(table)
{
    this->page_num = table->get_root_page_num();
    this->move(position);
}

void Cursor::move(CursorPosition position)
{
    switch (position)
    {
    case CursorPosition::BEGIN:
        move_begin();
        break;
    case CursorPosition::END:
        move_end();
        break;
    }
}

//
// Return the position of the given key.
// If the key is not present, return the position
// where it should be inserted
//
Cursor *Cursor::find(uint32_t key)
{
    uint32_t root_page_num = this->table->get_root_page_num();
    LeafNode *root_node = this->table->pager->get_page(root_page_num);

    if (root_node->get_node_type() == NodeType::LEAF)
    {
        return this->leaf_node_find(root_page_num, key);
    }
    else
    {
        std::cout << "Need to implement searching an internal node" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

Cursor *Cursor::leaf_node_find(uint32_t page_num, uint32_t key)
{
    LeafNode *node = this->table->pager->get_page(page_num);
    uint32_t num_cells = node->get_num_cells();

    // Binary search
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while (one_past_max_index != min_index)
    {
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = node->get_cell(index)->get_key();
        if (key == key_at_index)
        {
            this->cell_num = index;
            return this;
        }
        if (key < key_at_index)
        {
            one_past_max_index = index;
        }
        else
        {
            min_index = index + 1;
        }
    }

    this->cell_num = min_index;
    return this;
}

void Cursor::move_begin()
{
    this->cell_num = 0;
    this->end_of_table = (table->pager->get_page(table->get_root_page_num())->get_num_cells() == 0);
}

void Cursor::move_end()
{
    this->cell_num = table->pager->get_page(this->page_num)->get_num_cells();
    this->end_of_table = true;
}