#include "RedBlackTree.h"

int main(void) {
    RedBlackTree tree;
    rbt_init(&tree);

    rbt_insert(&tree, 10);
    rbt_insert(&tree, 5);
    rbt_insert(&tree, 5);
    rbt_insert(&tree, 5);
    rbt_insert(&tree, 5);
    rbt_insert(&tree, 5);
    rbt_insert(&tree, 5);
    rbt_insert(&tree, 15);
    rbt_insert(&tree, 3);
    rbt_insert(&tree, 7);
    rbt_insert(&tree, 7);
    rbt_insert(&tree, 12);
    rbt_insert(&tree, 18);

    printf("Red-Black Tree:\n");
    rbt_print(&tree);

    printf("\nRemoval 1: \n");
    RB_Node *alloc_node = rbt_remove(&tree, 5);
    printf("\n\nAfter deleting 5:\n");
    print_alloc(alloc_node); 
    rbt_print(&tree);
    
    printf("\nRemoval 2: \n");
    RB_Node *alloc_node1 = rbt_remove(&tree, 5);
    printf("\n\nAfter deleting 5:\n");
    print_alloc(alloc_node1); 
    rbt_print(&tree);

    printf("\nRemoval 3: \n");
    RB_Node *alloc_node2 = rbt_remove(&tree, 5);
    printf("\n\nAfter deleting 5:\n");
    print_alloc(alloc_node2); 
    rbt_print(&tree);

    printf("\nRemoval 4: \n");
    RB_Node *alloc_node3 = rbt_remove(&tree, 5);
    printf("\n\nAfter deleting 5:\n");
    print_alloc(alloc_node3); 
    rbt_print(&tree);

    
    printf("\nInsert back 4th Removal into tree");
    rbt_insert_node(&tree, alloc_node3);  
    rbt_print(&tree);
    print_alloc(alloc_node3); 

    printf("\nInsert back 3rd Removal into tree");
    rbt_insert_node(&tree, alloc_node2);  
    rbt_print(&tree);
    print_alloc(alloc_node2); 

    printf("\nInsert back 2nd Removal into tree");
    rbt_insert_node(&tree, alloc_node1);  
    rbt_print(&tree);
    print_alloc(alloc_node1); 

    printf("\nInsert back 1st Removal into tree");
    rbt_insert_node(&tree, alloc_node);  
    rbt_print(&tree);
    print_alloc(alloc_node); 

    return 0;
}
