#pragma once

#include <fstream>

const uint32_t COLUMN_USERNAME_SIZE = 32;
const uint32_t COLUMN_EMAIL_SIZE = 255;

struct Row
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];

    void print();
};

// padding may vary by complier and os
const uint32_t ID_SIZE = sizeof(Row::id);
const uint32_t USERNAME_SIZE = sizeof(Row::username);
// const uint32_t USERNAME_PADDING = (uint64_t)&((Row*)0)->email - (uint64_t)&((Row*)0)->username - USERNAME_SIZE;
const uint32_t USERNAME_PADDING = 0;
const uint32_t EMAIL_SIZE = sizeof(Row::email);
// const uint32_t EMAIL_PADDING = sizeof(Row) - (uint64_t)&((Row*)0)->email - EMAIL_SIZE;
const uint32_t EMAIL_PADDING = 3;
const uint32_t STRUCT_PADDING = USERNAME_PADDING + EMAIL_PADDING;

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE ;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE + USERNAME_PADDING;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE + STRUCT_PADDING;

const uint32_t TABLE_MAX_PAGES = 100;
const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;

class Cell
{
    uint32_t *key;
    Row *value;

public:

    Cell(uint32_t *key, Row *value);

    uint32_t get_key();
    void set_key(uint32_t key);

    Row *get_value();
    void set_value(Row *row);

    Cell *get_address();
};

enum class NodeType
{
    LEAF,
    INTERNAL
};

//
// Common Node Header Layout
//
const uint32_t NODE_TYPE_SIZE = sizeof(NodeType::INTERNAL);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(bool);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(nullptr_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint32_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

class Node
{
private:
    // Node Header
    NodeType *nodeType;

public:
    bool *isRoot;
    Node *parent;

    Node(NodeType *nodeType, bool *isRoot, Node *parent);

    NodeType get_node_type();
    void set_node_type(NodeType nodeType);
};

//
// Leaf Node Header Layout
//
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

//
// Leaf Node Body Layout
//
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

struct LeafNode : Node
{
private:
    // Node Header
    uint32_t *num_cells;

    // Node Body
    std::array<Cell *, LEAF_NODE_MAX_CELLS> cells;

public:
    LeafNode(NodeType *nodeType, bool *isRoot, Node *parent, uint32_t *num_cells);

    uint32_t get_num_cells();
    void set_num_cells(uint32_t num_cells);

    Cell *get_cell(uint32_t cell_num);
    void set_cell(uint32_t cell_num, Cell *cell);

    void print();
};

struct Pager
{
private:
    std::array<char *, TABLE_MAX_PAGES> page_data;
    std::array<LeafNode *, TABLE_MAX_PAGES> pages;

public:
    std::string filename;
    std::streampos file_length;
    uint32_t num_pages;

    Pager(std::string filename);
    ~Pager();

    void flush(uint32_t page_num);
    LeafNode *get_page(uint32_t page_num);
    LeafNode *deserialize(char *page_data);
    LeafNode *initialize_page(char *page_data);
};

struct Table
{
private:
    uint32_t root_page_num;

public:
    Pager *pager;

    Table(std::string filename);
    ~Table();

    uint32_t get_root_page_num();
};

enum class CursorPosition
{
    BEGIN,
    END
};

struct Cursor
{
    Table *table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates is the cursor locate in a position after the last element

    Cursor(Table *table, CursorPosition position);

    void move(CursorPosition position);

    Row *value();
    void advance();

    Cursor *find(uint32_t key);
    Cursor *leaf_node_find(uint32_t page_num, uint32_t key);
    void insert(uint32_t key, Row *value);

private:
    void move_begin();
    void move_end();
};