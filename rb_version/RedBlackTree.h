#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include <stdio.h>
#include <stdlib.h>

#include "arena.h"

/***  RB_TREE API ***/
void rbt_init(RB_Tree* tree);
void rbt_insert(RB_Tree* tree, int val);
void rbt_initial_insert_node(RB_Tree* tree, RB_Node *z);
int rbt_re_insert_node(RB_Tree* tree, RB_Node *z);
RB_Node *rbt_remove(RB_Tree* tree, int val);
void rbt_print(const RB_Tree* tree);
void print_alloc(RB_Node *node);
RB_Node *rbt_find(const RB_Tree *tree, int key);
RB_Node *clear_rb_node(RB_Node *node);
RB_Node *rbt_remove_node(RB_Tree *tree, RB_Node *node);


#endif
