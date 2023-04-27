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

LeafNodeCell::LeafNodeCell(uint32_t *key, Row *value)
    : key(key), value(value)
{
}

uint32_t LeafNodeCell::get_key()
{
    return *this->key;
}

void LeafNodeCell::set_key(uint32_t key)
{
    *this->key = key;
}

Row *LeafNodeCell::get_value()
{
    return this->value;
}

void LeafNodeCell::set_value(Row *row)
{
    // copy value of *row to *this->value
    // same as
    // memcpy(this->value, row, LEAF_NODE_VALUE_SIZE);
    // copying the elements one by one may avoid copying padding
    *this->value = *row;
}

LeafNodeCell *LeafNodeCell::get_address()
{
    return (LeafNodeCell *)this->key;
}

Node::Node(NodeType *nodeType, bool *isRoot, Node *parent)
    : nodeType(nodeType), isRoot(isRoot), parent(parent)
{
}

NodeType Node::get_node_type()
{
    return *this->nodeType;
}

bool Node::get_node_root()
{
    return *this->isRoot;
}

void Node::set_node_root(bool isRoot)
{
    *this->isRoot = isRoot;
}

uint32_t Node::get_max_key()
{
    uint32_t max;
    switch (this->get_node_type())
    {
    case NodeType::INTERNAL:
    {
        InternalNode *casted_internal = (InternalNode *)this;
        max = casted_internal->get_key(casted_internal->get_num_keys() - 1);
    }
    case NodeType::LEAF:
    {
        LeafNode *casted_leaf = (LeafNode *)this;
        max = casted_leaf->get_cell(casted_leaf->get_num_cells() - 1)->get_key();
    }
    }
    return max;
}

LeafNode::LeafNode(NodeType *nodeType, bool *isRoot, Node *parent, uint32_t *num_cells)
    : Node(nodeType, isRoot, parent), num_cells(num_cells)
{
}

LeafNode::~LeafNode()
{
    for (auto cell : this->cells)
    {
        delete cell;
    }
}

uint32_t LeafNode::get_num_cells()
{
    return *num_cells;
}

void LeafNode::set_num_cells(uint32_t num_cells)
{
    *this->num_cells = num_cells;
}

void LeafNode::set_num_cells(uint32_t *num_cells)
{
    this->num_cells = num_cells;
}

LeafNodeCell *LeafNode::get_cell(uint32_t cell_num)
{
    return this->cells[cell_num];
}

void LeafNode::set_cell(uint32_t cell_num, LeafNodeCell *cell)
{
    this->cells[cell_num] = cell;
}

void LeafNode::set_cell(uint32_t cell_num, uint32_t key, Row *row)
{
    LeafNodeCell *cell = this->cells[cell_num];
    cell->set_key(key);
    cell->set_value(row);
}

void LeafNode::copy_cell(uint32_t src_cell_num, uint32_t dst_cell_num)
{
    memcpy(this->get_cell(src_cell_num)->get_address(), this->get_cell(dst_cell_num)->get_address(), LEAF_NODE_CELL_SIZE);
}

InternalNodeCell::InternalNodeCell(uint32_t *key, uint32_t *value)
    : key(key), value(value)
{
}

uint32_t InternalNodeCell::get_key()
{
    return *this->key;
}

void InternalNodeCell::set_key(uint32_t key)
{
    *this->key = key;
}

uint32_t InternalNodeCell::get_value()
{
    return *this->value;
}

void InternalNodeCell::set_value(uint32_t child_num)
{
    *this->value = child_num;
}

InternalNode::InternalNode(NodeType *nodeType, bool *isRoot, Node *parent, uint32_t *num_keys, uint32_t *right_child_num)
    : Node(nodeType, isRoot, parent)
{
    this->num_keys = num_keys;
    this->right_child = right_child_num;
}

InternalNode::~InternalNode()
{
    for (auto cell : this->cells)
    {
        delete cell;
    }
}

uint32_t InternalNode::get_num_keys()
{
    return *this->num_keys;
}

uint32_t InternalNode::get_right_child()
{
    return *this->right_child;
}

InternalNodeCell *InternalNode::get_cell(uint32_t cell_num)
{
    return this->cells[cell_num];
}

uint32_t InternalNode::get_child(uint32_t child_num)
{
    uint32_t num_keys = this->get_num_keys();
    if (child_num > num_keys)
    {
        std::cout << "Tried to access child_num " << child_num << " > num_keys " << num_keys << std::endl;
        std::exit(EXIT_FAILURE);
    }
    else if (child_num == num_keys)
    {
        return this->get_right_child();
    }
    else
    {
        return this->get_cell(child_num)->get_value();
    }
}

uint32_t InternalNode::get_key(uint32_t key_num)
{
    return this->get_cell(key_num)->get_key();
}

void InternalNode::set_num_keys(uint32_t num_keys)
{
    *this->num_keys = num_keys;
}

void InternalNode::set_right_child(uint32_t child_num)
{
    *this->right_child = child_num;
}

void InternalNode::set_num_keys(uint32_t *num_keys)
{
    this->num_keys = num_keys;
}

void InternalNode::set_right_child(uint32_t *child_num)
{
    this->right_child = child_num;
}

void InternalNode::set_cell(uint32_t cell_num, InternalNodeCell *cell)
{
    this->cells[cell_num] = cell;
}

void InternalNode::set_cell(uint32_t cell_num, uint32_t key, uint32_t value)
{
    InternalNodeCell *cell = this->cells[cell_num];
    cell->set_key(key);
    cell->set_value(value);
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
    NodeType nodeType = *(NodeType *)(&page_data[NODE_TYPE_OFFSET]);

    // switch to create node
    // separate to different functions
    // to avoid conflict
    switch (nodeType)
    {
    case NodeType::INTERNAL:
        return this->deserialize_internal(page_data);
    case NodeType::LEAF:
        return this->deserialize_leaf(page_data);
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
// parent = 0x000000000
// num_cells = 0

InternalNode *Pager::deserialize_internal(char *page_data)
{
    InternalNode *node = new InternalNode(
        (NodeType *)(&page_data[NODE_TYPE_OFFSET]),
        (bool *)(&page_data[IS_ROOT_OFFSET]),
        (Node *)(&page_data[PARENT_POINTER_OFFSET]),
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
        (bool *)(&page_data[IS_ROOT_OFFSET]),
        (Node *)(&page_data[PARENT_POINTER_OFFSET]),
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

// Until we start recycling free pages, new pages will always
// go onto the end of the database file
uint32_t Pager::get_unused_page_num()
{
    return this->num_pages;
}

void Pager::clean_page(uint32_t page_num)
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

void Pager::copy_node_cell(LeafNode *src_node, uint32_t src_cell_num, LeafNode *dst_node, uint32_t dst_cell_num)
{
    memcpy(dst_node->get_cell(dst_cell_num)->get_address(), src_node->get_cell(src_cell_num)->get_address(), LEAF_NODE_CELL_SIZE);
}

// will clean page data after changing node type
Node *Pager::set_node_type(uint32_t page_num, NodeType new_type)
{
    Node *node = this->get_page(page_num);
    if (node->get_node_type() != new_type)
    {
        this->clean_page(page_num);
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

    NodeType nodeType = node->get_node_type();
    switch (nodeType)
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
    InternalNode *node_internal = (InternalNode *)node;
    num_keys = node_internal->get_num_keys();
    indent(indentation_level);
    std::cout << "- internal (size " << num_keys << ")" << std::endl;

    for (uint32_t i = 0; i < num_keys; i++)
    {
        child = node_internal->get_child(i);
        this->print_tree(child, indentation_level + 1);

        indent(indentation_level + 1);
        std::cout << "- key " << node_internal->get_key(i) << std::endl;
    }
    child = node_internal->get_right_child();
    this->print_tree(child, indentation_level + 1);
}

void Pager::print_leaf(Node *node, uint32_t indentation_level)
{
    uint32_t num_keys, child;
    LeafNode *node_leaf = (LeafNode *)node;
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

Table::Table(std::string filename)
{
    this->root_page_num = 0;
    this->pager = new Pager(filename);

    if (this->pager->num_pages == 0)
    {
        // New database file. Initialize page 0 as leaf node.
        LeafNode *root_node = (LeafNode *)this->pager->get_page(0);
        root_node->set_node_root(true);
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
        // TODO - FIX: first page hasn't saved - all zero
        pager->flush(i);
    }
    delete pager;
}

uint32_t Table::get_root_page_num()
{
    return this->root_page_num;
}

Node *Table::create_new_root(uint32_t right_child_page_num)
{
    // Handle splitting the root.
    // Old root copied to new page, becomes left child.
    // Address of right child passed in.
    // Re-initialize root page to contain the new root node.
    // New root node points to two children.

    LeafNode *old_root = (LeafNode *)this->pager->get_page(this->root_page_num);
    uint32_t left_child_page_num = this->pager->get_unused_page_num();
    LeafNode *left_child = (LeafNode *)this->pager->get_page(left_child_page_num);

    // Left child has data copied from old root
    this->pager->copy_node_data(left_child_page_num, root_page_num);
    left_child->set_node_root(false);

    // Root node is a new internal node with one key and two children
    InternalNode *new_root = (InternalNode *)this->pager->set_node_type(root_page_num, NodeType::INTERNAL);
    new_root->set_node_root(true);
    new_root->set_num_keys(1);
    new_root->set_right_child(right_child_page_num);
    new_root->set_cell(0, left_child->get_max_key(), left_child_page_num);
    return new_root;
}

Row *Cursor::value()
{
    LeafNode *node = (LeafNode *)this->table->pager->get_page(this->page_num);
    return node->get_cell(this->cell_num)->get_value();
}

void Cursor::advance()
{
    LeafNode *node = (LeafNode *)this->table->pager->get_page(this->page_num);
    this->cell_num += 1;
    // TODO: Next page
    if (this->cell_num >= node->get_num_cells())
    {
        this->end_of_table = true;
    }
}

void Cursor::insert(uint32_t key, Row *value)
{
    LeafNode *node = (LeafNode *)this->table->pager->get_page(this->page_num);

    uint32_t num_cells = node->get_num_cells();
    if (num_cells >= LEAF_NODE_MAX_CELLS)
    {
        // Node full
        this->split_and_insert(key, value);
        return;
    }

    if (this->cell_num < num_cells)
    {
        // Make room for new cell
        for (uint32_t i = num_cells; i > this->cell_num; i--)
        {
            node->copy_cell(i, i - 1);
        }
    }

    node->set_num_cells(num_cells + 1);
    LeafNodeCell *cell = node->get_cell(this->cell_num); // TODO: operator reload []
    cell->set_key(key);
    cell->set_value(value);
}

const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

Node *Cursor::split_and_insert(uint32_t key, Row *value)
{
    // Create a new node and move half the cells over.
    // Insert the new value in one of the two nodes.
    // Update parent or create a new parent.

    LeafNode *old_node = (LeafNode *)this->table->pager->get_page(this->page_num);
    uint32_t new_page_num = this->table->pager->get_unused_page_num();
    LeafNode *new_node = (LeafNode *)this->table->pager->get_page(new_page_num);

    // All existing keys plus new key should be divided
    // evenly between old (left) and new (right) nodes.
    // Starting from the right, move each key to correct position.

    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--)
    {
        LeafNode *destination_node;
        if (i >= LEAF_NODE_LEFT_SPLIT_COUNT)
        {
            destination_node = new_node;
        }
        else
        {
            destination_node = old_node;
        }

        if (i == this->cell_num)
        {
            LeafNodeCell *destination = destination_node->get_cell(i % LEAF_NODE_LEFT_SPLIT_COUNT);
            destination->set_key(key);
            destination->set_value(value);
        }
        else
        {
            this->table->pager->copy_node_cell(
                old_node, i > this->cell_num ? i - 1 : i,
                destination_node, i % LEAF_NODE_LEFT_SPLIT_COUNT);
        }
    }

    // Update cell count on both leaf nodes
    old_node->set_num_cells(LEAF_NODE_LEFT_SPLIT_COUNT);
    new_node->set_num_cells(LEAF_NODE_RIGHT_SPLIT_COUNT);

    // update root node
    if (old_node->get_node_root())
    {
        return this->table->create_new_root(new_page_num);
    }
    else
    {
        std::cout << "Need to implement updating parent after split" << std::endl;
        std::exit(EXIT_FAILURE);
    }
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
    Node *root_node = this->table->pager->get_page(this->page_num);

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
    LeafNode *node = (LeafNode *)this->table->pager->get_page(this->page_num);
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
    auto page = (LeafNode *)table->pager->get_page(table->get_root_page_num());
    this->end_of_table = (page->get_num_cells() == 0);
}

void Cursor::move_end()
{
    auto page = (LeafNode *)table->pager->get_page(this->page_num);
    this->cell_num = page->get_num_cells();
    this->end_of_table = true;
}