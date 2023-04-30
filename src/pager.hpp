#pragma once

#include <fstream>

#include "btree.hpp"

class Pager
{
private:
    // variables
    
    std::array<char *, TABLE_MAX_PAGES> page_data;
    std::array<Node *, TABLE_MAX_PAGES> pages;

    // functions

    char *new_page_data();

    InternalNode *deserialize_internal(char *page_data);
    LeafNode *deserialize_leaf(char *page_data);

    void print_internal(Node *node, uint32_t indentation_level);
    void print_leaf(Node *node, uint32_t indentation_level);

public:

    // variables

    std::string filename;
    std::streampos file_length;
    uint32_t num_pages;

    // functions

    Pager(std::string filename);
    ~Pager();

    void flush(uint32_t page_num);
    Node *get_page(uint32_t page_num);
    Node *deserialize(char *page_data);

    uint32_t get_unused_page_num();

    Node *set_node_type(uint32_t page_num, NodeType node_type);

    void clean_page(uint32_t page_num);

    void copy_node_data(uint32_t src_page_num, uint32_t dst_page_num);
    void copy_node_cell(LeafNode *src_node, uint32_t src_cell_num, LeafNode *dst_node, uint32_t dst_cell_num);

    void print_tree(uint32_t page_num, uint32_t indentation_level);
};