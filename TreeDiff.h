//#include <MyLib2.h>
#include <cmath>
#include <cstdio>

#ifndef DIFFERENTIATOR_TREEDIFF_H
#define DIFFERENTIATOR_TREEDIFF_H

typedef double number_t;

typedef struct data_tree {
    unsigned char type;         // Число, переменная, оператор +,-,sin,arcth
    number_t number;            // Число
} data_tree_t;

typedef data_tree_t data_t;

enum consts {
    RIGHT,
    LEFT,
};

enum errors {
    ALL_RIGHT,
    EFILLROOT,
    EFILLNODE,
    EREAD,
    EADDRIGHT,
    EADDLEFT,
    SYNERROR,
    EUNIDENT
};

typedef struct nodeTree {
public:
    data_t data;
    nodeTree *prev;
    nodeTree *right;
    nodeTree *left;

    // Аргумент aux_arg может исопльзоваться для передачи, например указателя на дерево этого узла
    int visitor_pref_r (int (*func) (nodeTree *, void *), void *aux_arg = nullptr);
} node_t;

typedef struct Tree {
    node_t *node;       // Указатель на корень

    Tree ();

    int verificator ();

    inline node_t *add_right (node_t *in_node, data_t value);

    inline node_t *add_left (node_t *in_node, data_t value);

    int read_expression (char *source);

    static node_t *crtNode (int type, number_t value, node_t *left, node_t *right, node_t *prev = nullptr);

    static node_t *get_Diff (node_t *to_node, node_t *curr_node, int how_many = 1);

    static node_t *Copy (const node_t *curr_node);

    static int Optimizer (node_t *head_node);

    static int del_Branch_Left (node_t *curr_node);     // Рекурентно удалить у переданного узла левую ветку

    static int del_Branch_Right (node_t *curr_node);    // Рекурентно удалить у переданного узла правую ветку

    static int del_Node_Left (node_t *curr_node);   // Вырезает узел с его левой частью из дерева, устраняя разрыв
    // Рекурентно удалить у переданного узла левую ветку, сам узел
    // и склеить правую ветку и родителя удалённого узла.
    static int del_Node_Right (node_t *curr_node);  // Вырезает узел с его правой частью из дерева, устраняя разрыв
    // Рекурентно удалить у переданного узла правую ветку, сам узел
    // и склеить левую ветку и родителя удалённого узла.

private:
    node_t *add (int right_or_left, node_t *curr_node, data_t value);

    static node_t *get_new_node ();

    static node_t *Diff (const node_t *curr_node);

    static int Opt (node_t *curr_node);

    static int set_Prev (node_t *curr_node);

    static int del_Branch (node_t *del_node);
} tree_t;

void Dump_Tree (FILE *file_out, node_t *head);

void Dump_Tree_img (node_t *head);

void Dump_Tree_Tex (node_t *head, char *name_file = nullptr);

#endif //DIFFERENTIATOR_TREEDIFF_H
