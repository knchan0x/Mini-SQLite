#pragma once

//
// Row
//

constexpr uint32_t COLUMN_USERNAME_SIZE = 32;
constexpr uint32_t COLUMN_EMAIL_SIZE = 255;

struct Row
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];

    void print();
};

//
// Page structure
// remarks: padding may vary by complier and os
//

constexpr uint32_t ID_SIZE = sizeof(Row::id);
constexpr uint32_t USERNAME_SIZE = sizeof(Row::username);

constexpr uint32_t USERNAME_PADDING = 0;
constexpr uint32_t EMAIL_SIZE = sizeof(Row::email);
constexpr uint32_t EMAIL_PADDING = 3;
constexpr uint32_t STRUCT_PADDING = USERNAME_PADDING + EMAIL_PADDING;

constexpr uint32_t ID_OFFSET = 0;
constexpr uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
constexpr uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE + USERNAME_PADDING;
constexpr uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE + STRUCT_PADDING;

constexpr uint32_t TABLE_MAX_PAGES = 100;
constexpr uint32_t PAGE_SIZE = 4096;
constexpr uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;

enum class NodeType
{
    LEAF, // default node type
    INTERNAL
};

//
// Common Node Header Layout
//

constexpr uint32_t NODE_TYPE_SIZE = sizeof(NodeType::INTERNAL);
constexpr uint32_t NODE_TYPE_OFFSET = 0;
constexpr uint32_t IS_ROOT_SIZE = sizeof(bool);
constexpr uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
constexpr uint32_t PARENT_NUM_SIZE = sizeof(uint32_t);
constexpr uint32_t PARENT_NUM_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
constexpr uint32_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_NUM_SIZE;

//
// Leaf Node Header Layout
//

constexpr uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
constexpr uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
constexpr uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NEXT_LEAF_SIZE;

//
// Leaf Node Body Layout
//

constexpr uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
constexpr uint32_t LEAF_NODE_KEY_OFFSET = 0;
constexpr uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
constexpr uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
constexpr uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
constexpr uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
constexpr uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

//
// Internal Node Header Layout
//
constexpr uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
constexpr uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
constexpr uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE;

//
// Internal Node Body Layout
//
constexpr uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(nullptr_t);
constexpr uint32_t INTERNAL_NODE_VALUE_SIZE = sizeof(nullptr_t);
constexpr uint32_t INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_KEY_SIZE + INTERNAL_NODE_VALUE_SIZE;
constexpr uint32_t INTERNAL_NODE_SPACE_FOR_CELLS = PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE;
constexpr uint32_t INTERNAL_NODE_MAX_CELLS = INTERNAL_NODE_SPACE_FOR_CELLS / INTERNAL_NODE_CELL_SIZE;

class Node
{
public:
    // functions

    Node(NodeType *nodeType, bool *isRoot, uint32_t *parent_num);
    virtual ~Node() = default;

    NodeType get_node_type();

    bool is_root();
    void set_root(bool isRoot);

    virtual uint32_t get_max_key() = 0;

    uint32_t get_parent();
    void set_parent(uint32_t page_num);

private:
    // variables

    //
    // Node Header
    //

    NodeType *nodeType;
    bool *isRoot;
    uint32_t *parent_num;
};

class LeafNodeCell
{
public:
    // functions

    LeafNodeCell(uint32_t *key, Row *value);

    uint32_t get_key();
    void set_key(uint32_t key);

    Row *get_value();
    void set_value(const Row &row);

    LeafNodeCell *get_address();

private:
    // variables

    uint32_t *key;
    Row *value;
};

class LeafNode : public Node
{
public:
    // functions

    LeafNode(NodeType *nodeType, uint32_t *next_leaf_num, bool *isRoot, uint32_t *parent_num, uint32_t *num_cells);
    ~LeafNode();

    uint32_t get_max_key() override;

    uint32_t get_num_cells();
    void set_num_cells(uint32_t num_cells);

    void set_cell(uint32_t index, LeafNodeCell *cell); // for initialize from page_data
    void set_cell(uint32_t index, uint32_t key, const Row &row);
    void set_next_leaf_num(uint32_t next_leaf_num);

    LeafNodeCell *get_cell(uint32_t index);
    uint32_t get_next_leaf();

    void copy_cell(uint32_t dst_index, uint32_t src_index);

private:
    // variables

    // Leaf Node Header

    uint32_t *num_cells;
    uint32_t *next_leaf_num;

    // Leaf Node Body

    std::array<LeafNodeCell *, LEAF_NODE_MAX_CELLS> cells;
};

class InternalNodeCell
{
public:
    // functions

    InternalNodeCell(uint32_t *key, uint32_t *value);

    uint32_t get_key();
    void set_key(uint32_t key);

    uint32_t get_value();
    void set_value(uint32_t child_num);

    InternalNodeCell *get_address();

private:
    // variables

    uint32_t *key;
    uint32_t *value;
};

class InternalNode : public Node
{
public:
    // functions

    InternalNode(NodeType *nodeType, bool *isRoot, uint32_t *parent_num, uint32_t *num_keys, uint32_t *right_child_num);
    ~InternalNode();

    uint32_t get_max_key() override;

    uint32_t get_num_keys();
    void set_num_keys(uint32_t num_keys);

    uint32_t get_right_child();
    void set_right_child(uint32_t child_num);

    InternalNodeCell *get_cell(uint32_t index);
    void set_cell(uint32_t index, InternalNodeCell *cell); // for initialize from page_data
    void set_cell(uint32_t index, uint32_t key, uint32_t child_num);

    uint32_t get_key_at_cell(uint32_t index);
    uint32_t get_child_at_cell(uint32_t index);

    uint32_t find_child(uint32_t key);

    void update_key(uint32_t old_key, uint32_t new_key);

    void copy_cell(uint32_t dst_index, uint32_t src_index);

private:
    // variables

    // Internal Node Header

    uint32_t *num_keys;
    uint32_t *right_child; // page number of its rightmost child

    // Internal Node Body

    std::array<InternalNodeCell *, INTERNAL_NODE_MAX_CELLS> cells;
};