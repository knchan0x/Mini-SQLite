#include <iostream>
#include <string>

#include "btree.hpp"

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

void LeafNodeCell::set_value(const Row &row)
{
    // = *this->value = row;
    // copying the elements one by one may avoid copying padding
    memcpy(this->value, &row, LEAF_NODE_VALUE_SIZE);
}

LeafNodeCell *LeafNodeCell::get_address()
{
    return (LeafNodeCell *)this->key;
}

Node::Node(NodeType *nodeType, bool *isRoot, uint32_t *parent_num)
    : nodeType(nodeType), isRoot(isRoot), parent_num(parent_num)
{
}

NodeType Node::get_node_type()
{
    return *this->nodeType;
}

bool Node::is_root()
{
    return *this->isRoot;
}

void Node::set_root(bool isRoot)
{
    *this->isRoot = isRoot;
}

uint32_t LeafNode::get_max_key()
{
    return this->get_cell(this->get_num_cells() - 1)->get_key();
}

uint32_t InternalNode::get_max_key()
{
    return this->get_key_at_cell(this->get_num_keys() - 1);
}

uint32_t Node::get_parent()
{
    return *this->parent_num;
}

void Node::set_parent(uint32_t page_num)
{
    *this->parent_num = page_num;
}

LeafNode::LeafNode(NodeType *nodeType, uint32_t *next_leaf_num, bool *isRoot, uint32_t *parent_num, uint32_t *num_cells)
    : Node(nodeType, isRoot, parent_num), num_cells(num_cells), next_leaf_num(next_leaf_num)
{
}

LeafNode::~LeafNode()
{
    for (auto &cell : this->cells)
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

void LeafNode::set_next_leaf_num(uint32_t next_leaf_num)
{
    *this->next_leaf_num = next_leaf_num;
}

LeafNodeCell *LeafNode::get_cell(uint32_t index)
{
    return this->cells[index];
}

uint32_t LeafNode::get_next_leaf()
{
    return *this->next_leaf_num;
}

void LeafNode::set_cell(uint32_t index, LeafNodeCell *cell)
{
    this->cells[index] = cell;
}

void LeafNode::set_cell(uint32_t index, uint32_t key, const Row &row)
{
    LeafNodeCell *cell = this->cells[index];
    cell->set_key(key);
    cell->set_value(row);
}

void LeafNode::copy_cell(uint32_t dst_index, uint32_t src_index)
{
    memcpy(this->get_cell(dst_index)->get_address(), this->get_cell(src_index)->get_address(), LEAF_NODE_CELL_SIZE);
}

InternalNodeCell::InternalNodeCell(uint32_t *key, uint32_t *value)
    : key(key), value(value)
{
}

InternalNodeCell *InternalNodeCell::get_address()
{
    return (InternalNodeCell *)this->key;
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

InternalNode::InternalNode(NodeType *nodeType, bool *isRoot, uint32_t *parent_num, uint32_t *num_keys, uint32_t *right_child_num)
    : Node(nodeType, isRoot, parent_num)
{
    this->num_keys = num_keys;
    this->right_child = right_child_num;
}

InternalNode::~InternalNode()
{
    for (auto &cell : this->cells)
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

InternalNodeCell *InternalNode::get_cell(uint32_t index)
{
    return this->cells[index];
}

uint32_t InternalNode::get_child_at_cell(uint32_t cell_num)
{
    uint32_t num_keys = this->get_num_keys();
    if (cell_num > num_keys)
    {
        throw std::out_of_range("cell num > num keys");
    }
    else if (cell_num == num_keys)
    {
        return this->get_right_child();
    }
    else
    {
        return this->get_cell(cell_num)->get_value();
    }
}

uint32_t InternalNode::get_key_at_cell(uint32_t index)
{
    return this->get_cell(index)->get_key();
}

void InternalNode::set_num_keys(uint32_t num_keys)
{
    *this->num_keys = num_keys;
}

void InternalNode::set_right_child(uint32_t child_num)
{
    *this->right_child = child_num;
}

void InternalNode::set_cell(uint32_t index, InternalNodeCell *cell)
{
    this->cells[index] = cell;
}

void InternalNode::set_cell(uint32_t index, uint32_t key, uint32_t child_num)
{
    InternalNodeCell *cell = this->cells[index];
    cell->set_key(key);
    cell->set_value(child_num);
}

void InternalNode::update_key(uint32_t old_key, uint32_t new_key)
{
    // find old node that contains old key
    // update the old key to new key
    uint32_t old_child_index = this->find_child(old_key);
    this->get_cell(old_child_index)->set_key(new_key);
}

void InternalNode::copy_cell(uint32_t dst_index, uint32_t src_index)
{
    memcpy(this->get_cell(dst_index)->get_address(), this->get_cell(src_index)->get_address(), INTERNAL_NODE_CELL_SIZE);
}

//
// Return the index of the child which should contain
// the given key.
//
uint32_t InternalNode::find_child(uint32_t key)
{
    uint32_t num_keys = this->get_num_keys();

    //
    // Binary search
    //
    
    uint32_t min_index = 0;
    uint32_t max_index = num_keys; // there is one more child than key

    while (min_index != max_index)
    {
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_right = this->get_key_at_cell(index);
        if (key_to_right >= key)
        {
            max_index = index;
        }
        else
        {
            min_index = index + 1;
        }
    }

    return min_index;
}