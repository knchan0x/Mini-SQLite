#include <iostream>

#include "vm.hpp"

Row *Cursor::value()
{
    LeafNode *node = (LeafNode *)this->table->pager->get_page(this->page_num);
    return node->get_cell(this->cell_num)->get_value();
}

void Cursor::advance()
{
    LeafNode *node = (LeafNode *)this->table->pager->get_page(this->page_num);
    this->cell_num += 1;
    if (this->cell_num >= node->get_num_cells())
    {
        if (node->get_next_leaf_num() == 0)
        {
            // This was rightmost leaf
            this->end_of_table = true;
        }
        else
        {
            this->page_num = node->get_next_leaf_num();
            this->cell_num = 0;
        }
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

void Cursor::split_and_insert(uint32_t key, Row *value)
{
    // Create a new node and move half the cells over.
    // Insert the new value in one of the two nodes.
    // Update parent or create a new parent.

    LeafNode *old_node = (LeafNode *)this->table->pager->get_page(this->page_num);
    uint32_t old_max = old_node->get_max_key();
    uint32_t new_page_num = this->table->pager->get_unused_page_num();
    LeafNode *new_node = (LeafNode *)this->table->pager->get_page(new_page_num);
    new_node->set_parent(old_node->get_parent());

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
                destination_node, i % LEAF_NODE_LEFT_SPLIT_COUNT,
                old_node, i > this->cell_num ? i - 1 : i);
        }
    }

    // Update cell count on both leaf nodes
    old_node->set_num_cells(LEAF_NODE_LEFT_SPLIT_COUNT);
    new_node->set_num_cells(LEAF_NODE_RIGHT_SPLIT_COUNT);

    // Update next leaf
    new_node->set_next_leaf_num(old_node->get_next_leaf_num());
    old_node->set_next_leaf_num(new_page_num);

    // Update root node
    if (old_node->get_node_root())
    {
        this->table->create_new_root(new_page_num);
    }
    else
    {
        uint32_t parent_page_num = old_node->get_parent();
        uint32_t new_max = old_node->get_max_key();
        auto *parent = (InternalNode *)this->table->pager->get_page(parent_page_num);

        parent->update_key(old_max, new_max);
        this->table->internal_node_insert(parent_page_num, new_page_num);
    }
}

Cursor::Cursor(Table *table)
    : table(table)
{
    this->page_num = table->get_root_page_num();
    this->move(CursorPosition::BEGIN);
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
// Set the cursor to the position of the given key
// and return the position.
// If the key is not present, set the cursor to
// the position where it should be inserted
// and return the position.
//
Cursor *Cursor::find(uint32_t key)
{
    uint32_t root_page_num = this->table->get_root_page_num();
    Node *root_node = this->table->pager->get_page(root_page_num);

    if (root_node->get_node_type() == NodeType::LEAF)
    {
        return this->leaf_node_find(root_page_num, key);
    }
    else
    {
        return this->internal_node_find(root_page_num, key);
    }
}

Cursor *Cursor::leaf_node_find(uint32_t page_num, uint32_t key)
{
    LeafNode *node = (LeafNode *)this->table->pager->get_page(page_num);
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
            this->page_num = page_num;
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

    this->page_num = page_num;
    this->cell_num = min_index;
    return this;
}

Cursor *Cursor::internal_node_find(uint32_t page_num, uint32_t key)
{
    InternalNode *node = (InternalNode *)this->table->pager->get_page(page_num);

    uint32_t child_index = node->find_child(key);
    uint32_t child_num = node->get_child(child_index);
    Node *child = this->table->pager->get_page(child_num);

    switch (child->get_node_type())
    {
    case NodeType::LEAF:
        return this->leaf_node_find(child_num, key);
    case NodeType::INTERNAL:
        return this->internal_node_find(child_num, key);
    }
}

void Cursor::move_begin()
{
    this->find(0);
    auto *node = (LeafNode *)this->table->pager->get_page(this->page_num);
    this->end_of_table = (node->get_num_cells() == 0);
}

void Cursor::move_end()
{
    std::cout << "Not yet implemented move_end for cursor." << std::endl;
    std::exit(EXIT_FAILURE);
}

VirtualMachine::VirtualMachine(Table *table)
    : table(table)
{
}

ExecuteResult VirtualMachine::execute(Statement *statement)
{
    switch (statement->type)
    {
    case StatementType::EXIT:
        return ExecuteResult::EXIT;
    case StatementType::TREE:
        return this->print_tree();
    case StatementType::CONSTANTS:
        return this->print_constants();
    case StatementType::INSERT:
        return this->execute_insert(statement);
    case StatementType::SELECT:
        return this->execute_select(statement);
    }
}

ExecuteResult VirtualMachine::print_tree()
{
    std::cout << "Tree:" << std::endl;
    InternalNode *node = (InternalNode *)table->pager->get_page(0);
    table->pager->print_tree(0, 0);
    return ExecuteResult::SUCCESS;
}

ExecuteResult VirtualMachine::print_constants()
{
    std::cout << "Constants:" << std::endl;

    std::cout << "ROW_SIZE: " << ROW_SIZE << std::endl;

    std::cout << "COMMON_NODE_HEADER_SIZE: " << COMMON_NODE_HEADER_SIZE << std::endl;

    std::cout << "INTERNAL_NODE_HEADER_SIZE: " << INTERNAL_NODE_HEADER_SIZE << std::endl;
    std::cout << "INTERNAL_NODE_CELL_SIZE: " << INTERNAL_NODE_CELL_SIZE << std::endl;
    std::cout << "INTERNAL_NODE_SPACE_FOR_CELLS: " << INTERNAL_NODE_SPACE_FOR_CELLS << std::endl;
    std::cout << "INTERNAL_NODE_MAX_CELLS: " << INTERNAL_NODE_MAX_CELLS << std::endl;

    std::cout << "LEAF_NODE_HEADER_SIZE: " << LEAF_NODE_HEADER_SIZE << std::endl;
    std::cout << "LEAF_NODE_CELL_SIZE: " << LEAF_NODE_CELL_SIZE << std::endl;
    std::cout << "LEAF_NODE_SPACE_FOR_CELLS: " << LEAF_NODE_SPACE_FOR_CELLS << std::endl;
    std::cout << "LEAF_NODE_MAX_CELLS: " << LEAF_NODE_MAX_CELLS << std::endl;

    return ExecuteResult::SUCCESS;
}

ExecuteResult VirtualMachine::execute_insert(Statement *statement)
{
    auto cursor = std::make_unique<Cursor>(Cursor(this->table));
    Row *row_to_insert = &(statement->row_to_insert);
    uint32_t key_to_insert = row_to_insert->id;
    cursor->find(key_to_insert);

    auto page = (LeafNode *)cursor->table->pager->get_page(cursor->page_num);
    uint32_t num_cells = page->get_num_cells();
    if (cursor->cell_num < num_cells)
    {
        uint32_t key_at_index = page->get_cell(cursor->cell_num)->get_key();
        if (key_at_index == key_to_insert)
        {
            return ExecuteResult::DUPLICATE_KEY;
        }
    }

    cursor->insert(key_to_insert, row_to_insert);

    return ExecuteResult::SUCCESS;
}

ExecuteResult VirtualMachine::execute_select(Statement *statement)
{
    auto cursor = std::make_unique<Cursor>(Cursor(this->table));
    while (!cursor->end_of_table)
    {
        auto *page = (LeafNode *)(cursor->table->pager->get_page(cursor->page_num));
        page->get_cell(cursor->cell_num)->get_value()->print();
        cursor->advance();
    };
    return ExecuteResult::SUCCESS;
}