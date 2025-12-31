#include "RedBlackTree.h"

static size_t test_counter = 0;
/* ---------- Utility ---------- */

// static RB_Node* create_node(int val) {
//     RB_Node* n = (Node*)malloc(sizeof(Node));
//     n->data = val;
//     n->color = RED;
//     n->left = n->right = n->parent = NULL;
//     return n;
// }

//THIS WILL NOT NEED ALLOCATION IN PRGRAM
static RB_Node *create_node(int val) {
    RB_Node *n = (RB_Node*)malloc(sizeof(RB_Node));
    n->assoc_c_h_addr = (void *)test_counter;
    test_counter++; 
    n->size = val;
    n->color = RED;
    n->left = n->right = n->parent = NULL;
    return n;
}

static RB_Node *clear_node(RB_Node *node){
    node->color = RED;
    node->left = node->right = node->parent = NULL;
    return node;
}

/* ---------- Rotations ---------- */

static void left_rotate(RedBlackTree* tree, RB_Node* x) {
    RB_Node* y = x->right;
    if (!y) return;

    x->right = y->left;
    if (y->left)
        y->left->parent = x;

    y->parent = x->parent;

    if (!x->parent)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    y->left = x;
    x->parent = y;
}

static void right_rotate(RedBlackTree* tree, RB_Node* y) {
    RB_Node* x = y->left;
    if (!x) return;

    y->left = x->right;
    if (x->right)
        x->right->parent = y;

    x->parent = y->parent;

    if (!y->parent)
        tree->root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;

    x->right = y;
    y->parent = x;
}

/* ---------- Fix Insert ---------- */

static void fix_insert(RedBlackTree* tree, RB_Node* z) {
    while (z != tree->root && z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            RB_Node* y = z->parent->parent->right;

            if (y && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate(tree, z->parent->parent);
            }
        } else {
            RB_Node* y = z->parent->parent->left;

            if (y && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

/* ---------- Insert ---------- */
void rbt_insert(RedBlackTree* tree, int val) {
    RB_Node* z = create_node(val);
    RB_Node* y = NULL;
    RB_Node* x = tree->root;

    while (x) {
        y = x;
        if (val < x->size)
            x = x->left;
        else
            x = x->right;
    }

    z->parent = y;

    if (!y)
        tree->root = z;
    else if (val < y->size)
        y->left = z;
    else
        y->right = z;

    fix_insert(tree, z);
}

void rbt_insert_node(RedBlackTree* tree, RB_Node *z) {
    int val = z->size;
    RB_Node* y = NULL;
    RB_Node* x = tree->root;

    while (x) {
        y = x;
        //if (val < x->size)
        if ((void *)z < (void *)x)
            x = x->left;
        else
            x = x->right;
    }

    z->parent = y;

    if (!y)
        tree->root = z;
    //else if (val < y->size)
    else if ((void *)z < (void *)y)
        y->left = z;
    else
        y->right = z;

    fix_insert(tree, z);
    return;
}

/* ---------- Minimum ---------- */

static RB_Node* minimum(RB_Node* node) {
    while (node->left)
        node = node->left;
    return node;
}

/* ---------- Transplant ---------- */

static void transplant(RedBlackTree* tree, RB_Node* u, RB_Node* v) {
    if (!u->parent)
        tree->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;

    if (v)
        v->parent = u->parent;
}

/* ---------- Fix Delete ---------- */

static void fix_delete(RedBlackTree* tree, RB_Node* x) {
    while (x != tree->root && x && x->color == BLACK) {
        if (x == x->parent->left) {
            RB_Node* w = x->parent->right;

            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                left_rotate(tree, x->parent);
                w = x->parent->right;
            }

            if ((!w->left || w->left->color == BLACK) &&
                (!w->right || w->right->color == BLACK)) {
                w->color = RED;
                x = x->parent;
            } else {
                if (!w->right || w->right->color == BLACK) {
                    if (w->left)
                        w->left->color = BLACK;
                    w->color = RED;
                    right_rotate(tree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                if (w->right)
                    w->right->color = BLACK;
                left_rotate(tree, x->parent);
                x = tree->root;
            }
        } else {
            RB_Node* w = x->parent->left;

            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                right_rotate(tree, x->parent);
                w = x->parent->left;
            }

            if ((!w->right || w->right->color == BLACK) &&
                (!w->left || w->left->color == BLACK)) {
                w->color = RED;
                x = x->parent;
            } else {
                if (!w->left || w->left->color == BLACK) {
                    if (w->right)
                        w->right->color = BLACK;
                    w->color = RED;
                    left_rotate(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                if (w->left)
                    w->left->color = BLACK;
                right_rotate(tree, x->parent);
                x = tree->root;
            }
        }
    }
    if (x)
        x->color = BLACK;
}

/* ---------- Delete ---------- */

//static void delete_node(RedBlackTree* tree, RB_Node* z) {
static RB_Node *delete_node(RedBlackTree* tree, RB_Node* z) {
    RB_Node* y = z;
    RB_Node* x;
    Color y_original_color = y->color;

    if (!z->left) {
        x = z->right;
        transplant(tree, z, z->right);
    } else if (!z->right) {
        x = z->left;
        transplant(tree, z, z->left);
    } else {
        y = minimum(z->right);
        y_original_color = y->color;
        x = y->right;

        if (y->parent == z) {
            if (x)
                x->parent = y;
        } else {
            transplant(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }

        transplant(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    if (y_original_color == BLACK && x)
        fix_delete(tree, x);

    //free(z);
    clear_node(z);
    return z;
}

RB_Node *rbt_remove(RedBlackTree* tree, int val) {
    RB_Node* z = tree->root;

    while (z) {
        if (val < z->size)
            z = z->left;
        else if (val > z->size)
            z = z->right;
        else {
            RB_Node *alloc_node = delete_node(tree, z);
            //PRINT ALLOC_NODE INFO!!!
            print_alloc(alloc_node);
            return alloc_node;
        }
    }

    printf("RB_Node %d not found\n", val);
}
// typedef struct RB_Node {
//     int data;
//     Color color;
//     struct RB_Node* left;
//     struct RB_Node* right;
//     struct RB_Node* parent;
// } RB_Node;

// typedef struct RB_RB_Node{
//     Chunk_Header *assoc_c_h_addr;     //associated Chunk_Header Address
//     size_t rb_node_num; //!!! only needed for linked list version
//     size_t size;
//     struct RB_Node *left;
//     struct RB_Node *right;
//     struct RB_Node *parent;
//     Color color;
// }RB_Node;


/*------------Print Alloc'd RB_Node----------------*/
void print_alloc(RB_Node *node){
    printf("\nFREED NODE ADDR: %p\n", node);
    printf("ASSOC CHUNK HEADER &: %p\n", node->assoc_c_h_addr);
    printf("size: %ld\n", node->size);
    printf("left addr: %p\n", node->left);
    printf("right addr: %p\n", node->right);
    printf("parent addr: %p\n", node->parent);
    printf("Color: ");
    if(node->color == RED){
        printf("RED\n");
    }
    else{
        printf("BLACK\n");
    }
    return;
}

/* ---------- Print ---------- */

static void print_helper(RB_Node* root, int space) {
    const int COUNT = 5;
    if (!root) return;

    space += COUNT;
    print_helper(root->right, space);

    printf("\n");
    for (int i = COUNT; i < space; i++)
        printf(" ");

    printf("%ld(%s)", root->size,
           root->color == RED ? "RED" : "BLACK");
    printf("\n");

    print_helper(root->left, space);
}

void rbt_print(const RedBlackTree* tree) {
    print_helper(tree->root, 0);
}

/* ---------- Init ---------- */

void rbt_init(RedBlackTree* tree) {
    tree->root = NULL;
}
