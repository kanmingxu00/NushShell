#ifndef NODE_H
#define NODE_H

typedef struct node {
    char* op;
    struct node* tok_one;
    struct node* tok_two;
    char* command;
    int count;
    int used;
} node;


void free_nodes(node* tokens);

#endif
