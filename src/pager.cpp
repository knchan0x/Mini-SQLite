#include <iostream>
#include <string>

#include "pager.hpp"

Pager::Pager(const std::string &filename)
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
    for (auto& page : this->pages)
    {
        if (page)
        {
            delete page;
        }
    }
    for (auto& data : this->page_data)
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

    std::ofstream file;
    if (page_num == 0)
    {
        file = std::ofstream(filename, std::ios::trunc | std::ios::binary);
    }
    else
    {
        file = std::ofstream(filename, std::ios::app | std::ios::binary);
    }

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

Node *Pager::get_page(uint32_t page_num)
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

        char *data = this->new_page_data();
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

char *Pager::new_page_data()
{
    char *data = new char[PAGE_SIZE]();
    return data;
}

Node *Pager::deserialize(char *page_data)
{
    // switch to create node
    // separate to different functions
    // to avoid conflict
    switch (*(NodeType *)(&page_data[NODE_TYPE_OFFSET]))
    {
    case NodeType::INTERNAL:
        return (Node *)this->deserialize_internal(page_data);
    case NodeType::LEAF:
        return (Node *)this->deserialize_leaf(page_data);
    }
}

// page_data is a char[]
// all elements set to 0 by default
// LeafNode constructed by pointer to
// the array, so the default value
// of those elements are all set to 0
// i.e.
// nodeType = NodeType::Leaf (=0)
// isRoot = false (=0)
// parent_num = 0
// num_cells = 0

InternalNode *Pager::deserialize_internal(char *page_data)
{
    InternalNode *node = new InternalNode(
        (NodeType *)(&page_data[NODE_TYPE_OFFSET]),
        (bool *)(&page_data[IS_ROOT_OFFSET]),
        (uint32_t *)(&page_data[PARENT_NUM_OFFSET]),
        (uint32_t *)(&page_data[INTERNAL_NODE_NUM_KEYS_OFFSET]),
        (uint32_t *)(&page_data[INTERNAL_NODE_RIGHT_CHILD_OFFSET]));

    uint32_t body_start = INTERNAL_NODE_HEADER_SIZE;
    for (uint32_t i = 0; i < INTERNAL_NODE_MAX_CELLS; i++)
    {
        uint32_t cell_start = body_start + i * INTERNAL_NODE_CELL_SIZE;
        InternalNodeCell *cell = new InternalNodeCell(
            (uint32_t *)(&page_data[cell_start + LEAF_NODE_KEY_OFFSET]),
            (uint32_t *)(&page_data[cell_start + LEAF_NODE_VALUE_OFFSET]));
        node->set_cell(i, cell);
    }
    return node;
}

LeafNode *Pager::deserialize_leaf(char *page_data)
{
    LeafNode *node = new LeafNode(
        (NodeType *)(&page_data[NODE_TYPE_OFFSET]),
        (uint32_t *)(&page_data[LEAF_NODE_NEXT_LEAF_OFFSET]),
        (bool *)(&page_data[IS_ROOT_OFFSET]),
        (uint32_t *)(&page_data[PARENT_NUM_OFFSET]),
        (uint32_t *)(&page_data[LEAF_NODE_NUM_CELLS_OFFSET]));

    uint32_t body_start = LEAF_NODE_HEADER_SIZE;
    for (uint32_t i = 0; i < LEAF_NODE_MAX_CELLS; i++)
    {
        uint32_t cell_start = body_start + i * LEAF_NODE_CELL_SIZE;
        LeafNodeCell *cell = new LeafNodeCell(
            (uint32_t *)(&page_data[cell_start + LEAF_NODE_KEY_OFFSET]),
            (Row *)(&page_data[cell_start + LEAF_NODE_VALUE_OFFSET]));
        node->set_cell(i, cell);
    }
    return node;
}

uint32_t Pager::get_page_num()
{
    return this->num_pages;
}

// Until we start recycling free pages, new pages will always
// go onto the end of the database file
uint32_t Pager::get_unused_page_num()
{
    return this->num_pages;
}

void Pager::clean_page_data(uint32_t page_num)
{
    char *old_page = this->page_data[page_num];
    char *new_page = this->new_page_data();
    this->page_data[page_num] = new_page;
    delete old_page;
}

void Pager::copy_node_data(uint32_t dst_page_num, uint32_t src_page_num)
{
    // deep copy
    memcpy(this->page_data[dst_page_num], this->page_data[src_page_num], PAGE_SIZE);
}

void Pager::copy_node_cell(LeafNode *dst_node, uint32_t dst_cell_num, LeafNode *src_node, uint32_t src_cell_num)
{
    memcpy(dst_node->get_cell(dst_cell_num)->get_address(), src_node->get_cell(src_cell_num)->get_address(), LEAF_NODE_CELL_SIZE);
}

// will clean page data after changing node type
Node *Pager::set_node_type(uint32_t page_num, NodeType new_type)
{
    Node *node = this->get_page(page_num);
    if (node->get_node_type() != new_type)
    {
        this->clean_page_data(page_num);
        // set node type
        char *page_data = this->page_data[page_num];
        NodeType *node_type = (NodeType *)(&page_data[NODE_TYPE_OFFSET]);
        *node_type = new_type;

        return this->get_page(page_num);
    }
    return node;
}

void indent(uint32_t level)
{
    for (uint32_t i = 0; i < level; i++)
    {
        std::cout << "  ";
    }
}

void Pager::print_tree(uint32_t page_num, uint32_t indentation_level)
{
    Node *node = this->get_page(page_num);

    switch (node->get_node_type())
    {
    case NodeType::INTERNAL:
        print_internal(node, indentation_level);
        break;
    case NodeType::LEAF:
        print_leaf(node, indentation_level);
        break;
    }
}

void Pager::print_internal(Node *node, uint32_t indentation_level)
{
    uint32_t num_keys, child;
    auto node_internal = static_cast<InternalNode *>(node);
    num_keys = node_internal->get_num_keys();
    indent(indentation_level);
    std::cout << "- internal (size " << num_keys << ")" << std::endl;

    for (uint32_t i = 0; i < num_keys; i++)
    {
        child = node_internal->get_child_at_cell(i);
        this->print_tree(child, indentation_level + 1);

        indent(indentation_level + 1);
        std::cout << "- key " << node_internal->get_key_at_cell(i) << std::endl;
    }
    child = node_internal->get_right_child();
    this->print_tree(child, indentation_level + 1);
}

void Pager::print_leaf(Node *node, uint32_t indentation_level)
{
    uint32_t num_keys, child;
    auto node_leaf = static_cast<LeafNode *>(node);
    num_keys = node_leaf->get_num_cells();
    indent(indentation_level);
    std::cout << "- leaf (size " << num_keys << ")" << std::endl;

    for (uint32_t i = 0; i < num_keys; i++)
    {
        indent(indentation_level + 1);
        std::cout << "- " << node_leaf->get_cell(i)->get_key()
                  << ": " << node_leaf->get_cell(i)->get_value()->username
                  << "  " << node_leaf->get_cell(i)->get_value()->email << std::endl;
    }
}