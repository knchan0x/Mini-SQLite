#include <iostream>
#include <string>

#include "table.hpp"

Table::Table(const std::string &filename)
{
    this->root_page_num = 0;
    this->pager = new Pager(filename);
    
    if (this->pager->get_page_num() == 0)
    {
        // New database file. Initialize page 0 as leaf node.
        Node *root_node = this->pager->get_page(0);
        root_node->set_root(true);
    }
}

Table::~Table()
{
    for (uint32_t i = 0; i < pager->get_page_num(); i++)
    {
        if (pager->get_page(i) == nullptr)
        {
            continue;
        }
        try
        {
            pager->flush(i);
        }
        catch(const std::runtime_error &e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
    delete pager;
}

uint32_t Table::get_root()
{
    return this->root_page_num;
}

Node &Table::new_root(uint32_t page_num)
{
    // Handle splitting the root.
    // Old root copied to new page, becomes left child.
    // Address of right child passed in.
    // Re-initialize root page to contain the new root node.
    // New root node points to two children.

    auto old_root = static_cast<LeafNode *>(this->pager->get_page(this->root_page_num));
    uint32_t left_child_page_num = this->pager->get_unused_page_num();
    auto left_child = static_cast<LeafNode *>(this->pager->get_page(left_child_page_num));

    // Left child has data copied from old root
    this->pager->copy_node_data(left_child_page_num, root_page_num);
    left_child->set_root(false);

    // Root node is a new internal node with one key and two children
    auto new_root = static_cast<InternalNode *>(this->pager->set_node_type(root_page_num, NodeType::INTERNAL));
    new_root->set_root(true);
    new_root->set_num_keys(1);
    new_root->set_right_child(page_num);
    new_root->set_cell(0, left_child->get_max_key(), left_child_page_num);
    return *new_root;
}