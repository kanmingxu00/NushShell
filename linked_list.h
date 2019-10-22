#ifndef LINKED_LIST_H
#define LINKED_LIST_H

// for linked list, used lecture notes for guidance (no copy paste)

// struct definition linked_list for linked_list of tokens
typedef struct linked_list {
    char* head;
    struct linked_list* tail;
} linked_list;

// puts a head onto the linked list, increases size
linked_list* cons(char* token, linked_list* tokens);
// frees a linked list entirely
void free_linked_list(linked_list* tokens);
// prints entirity of a linked list, starting with later inputs first
void print_linked_list(linked_list* tokens);

long length(linked_list* xs);
linked_list* reverse(linked_list* xs);
linked_list* rev_free(linked_list* xs);

#endif
