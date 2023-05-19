#include <iostream>

#include "vm.hpp"

Cursor::Cursor(Table &table)
    : table(table)
{
    this->page_num = this->table.get_root();
    this->move_begin();
}

void Cursor::move_begin()
{
    this->find(0);
    auto node = static_cast<LeafNode *>(this->table.pager->get_page(this->page_num));
    this->end_of_table = (node->get_num_cells() == 0);
}

void Cursor::advance()
{
    auto node = static_cast<LeafNode *>(this->table.pager->get_page(this->page_num));
    this->cell_num += 1;
    if (this->cell_num >= node->get_num_cells())
    {
        if (node->get_next_leaf() == 0)
        {
            // This was rightmost leaf
            this->end_of_table = true;
        }
        else
        {
            this->page_num = node->get_next_leaf();
            this->cell_num = 0;
        }
    }
}

void Cursor::insert(uint32_t key, const Row& value)
{
    auto node = static_cast<LeafNode *>(this->table.pager->get_page(this->page_num));

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
    LeafNodeCell *cell = node->get_cell(this->cell_num);
    cell->set_key(key);
    cell->set_value(value);
}

constexpr uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
constexpr uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

void Cursor::split_and_insert(uint32_t key, const Row& value)
{
    // Create a new node and move half the cells over.
    // Insert the new value in one of the two nodes.
    // Update parent or create a new parent.

    auto old_node = static_cast<LeafNode *>(this->table.pager->get_page(this->page_num));
    uint32_t old_max = old_node->get_max_key();
    uint32_t new_page_num = this->table.pager->get_unused_page_num();
    auto new_node = static_cast<LeafNode *>(this->table.pager->get_page(new_page_num));
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
            this->table.pager->copy_node_cell(
                destination_node, i % LEAF_NODE_LEFT_SPLIT_COUNT,
                old_node, i > this->cell_num ? i - 1 : i);
        }
    }

    // Update cell count on both leaf nodes
    old_node->set_num_cells(LEAF_NODE_LEFT_SPLIT_COUNT);
    new_node->set_num_cells(LEAF_NODE_RIGHT_SPLIT_COUNT);

    // Update next leaf
    new_node->set_next_leaf_num(old_node->get_next_leaf());
    old_node->set_next_leaf_num(new_page_num);

    // Update root node
    if (old_node->is_root())
    {
        this->table.new_root(new_page_num);
    }
    else
    {
        uint32_t parent_page_num = old_node->get_parent();
        uint32_t new_max = old_node->get_max_key();
        auto parent = static_cast<InternalNode *>(this->table.pager->get_page(parent_page_num));

        parent->update_key(old_max, new_max);
        this->insert_internal_node(parent_page_num, new_page_num);
    }
}

void Cursor::insert_internal_node(uint32_t parent_page_num, uint32_t child_page_num)
{
    //
    // Add a new child/key pair to parent that corresponds to child
    //

    auto parent = static_cast<InternalNode *>(this->table.pager->get_page(parent_page_num));
    auto child = static_cast<LeafNode *>(this->table.pager->get_page(child_page_num));
    uint32_t child_max_key = child->get_max_key();
    uint32_t index = parent->find_child(child_max_key);

    uint32_t original_num_keys = parent->get_num_keys();
    parent->set_num_keys(original_num_keys + 1);

    if (original_num_keys >= INTERNAL_NODE_MAX_CELLS)
    {
        std::cout << "Need to implement splitting internal node" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    uint32_t right_child_page_num = parent->get_right_child();
    auto right_child = static_cast<LeafNode *>(this->table.pager->get_page(right_child_page_num));

    if (child_max_key > right_child->get_max_key())
    {
        // Replace right child
        parent->set_cell(original_num_keys, right_child->get_max_key(), right_child_page_num);
        parent->set_right_child(child_page_num);
    }
    else
    {
        // Make room for the new cell
        for (uint32_t i = original_num_keys; i > index; i--)
        {
            parent->copy_cell(i, i - 1);
        }
        parent->set_cell(index, child_max_key, child_page_num);
    }
}

//
// Set the cursor to the position of the given key.
// If the key is not present, set the cursor to the position
// where it should be inserted.
//
void Cursor::find(uint32_t key)
{
    uint32_t root_page_num = this->table.get_root();
    Node *root_node = this->table.pager->get_page(root_page_num);

    if (root_node->get_node_type() == NodeType::LEAF)
    {
        this->leaf_node_find(root_page_num, key);
    }
    else
    {
        this->internal_node_find(root_page_num, key);
    }
}

void Cursor::leaf_node_find(uint32_t page_num, uint32_t key)
{
    auto node = static_cast<LeafNode *>(this->table.pager->get_page(page_num));
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
            return;
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
}

void Cursor::internal_node_find(uint32_t page_num, uint32_t key)
{
    auto node = static_cast<InternalNode *>(this->table.pager->get_page(page_num));

    uint32_t child_index = node->find_child(key);
    uint32_t child_num = node->get_child_at_cell(child_index);
    Node *child = this->table.pager->get_page(child_num);

    switch (child->get_node_type())
    {
    case NodeType::LEAF:
        this->leaf_node_find(child_num, key);
        break;
    case NodeType::INTERNAL:
        this->internal_node_find(child_num, key);
        break;
    }
}

uint32_t Cursor::get_page_num() {
    return this->page_num;
}

uint32_t Cursor::get_cell_num() {
    return this->cell_num;
}

bool Cursor::is_end_of_table() {
    return this->end_of_table;
}

VirtualMachine::VirtualMachine(Table *table) : table(table) {}

ExecuteResult VirtualMachine::execute(const Statement& statement)
{
    switch (statement.type)
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
    auto node = static_cast<InternalNode *>(this->table->pager->get_page(0));
    this->table->pager->print_tree(0, 0);
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

ExecuteResult VirtualMachine::execute_insert(const Statement& statement)
{
    auto cursor = std::make_unique<Cursor>(Cursor(*this->table));
    uint32_t key_to_insert = statement.row_to_insert.id;
    cursor->find(key_to_insert);

    auto page = static_cast<LeafNode *>(cursor->table.pager->get_page(cursor->get_page_num()));
    if (cursor->get_cell_num() < page->get_num_cells())
    {
        uint32_t key_at_index = page->get_cell(cursor->get_cell_num())->get_key();
        if (key_at_index == key_to_insert)
        {
            return ExecuteResult::DUPLICATE_KEY;
        }
    }

    cursor->insert(key_to_insert, statement.row_to_insert);

    return ExecuteResult::SUCCESS;
}

ExecuteResult VirtualMachine::execute_select(const Statement& statement)
{
    auto cursor = std::make_unique<Cursor>(Cursor(*this->table));
    while (!cursor->is_end_of_table())
    {
        auto page = static_cast<LeafNode *>((cursor->table.pager->get_page(cursor->get_page_num())));
        page->get_cell(cursor->get_cell_num())->get_value()->print();
        cursor->advance();
    };
    return ExecuteResult::SUCCESS;
}