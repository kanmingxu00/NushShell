#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linked_list.h"

// for linked list, used the lecture notes for guidance (no copy paste)

// allocates memory for the new linked list to return,
// puts the first token into the head, and old tokens into tail
linked_list*
cons(char* token, linked_list* tokens)
{
    linked_list* tokens_return = malloc(sizeof(linked_list));
    tokens_return->head = strdup(token);
    tokens_return->tail = tokens;
    return tokens_return;
}

// frees a linked list in its entirity, recursively
// frees the head, and then recurs on tail to free all the heads
void
free_linked_list(linked_list* tokens)
{
    if (tokens) {
        free_linked_list(tokens->tail);
        free(tokens->head);
        free(tokens);
    }
}

// prints the heads, starting with later inputs first
// effectively "backwards"
void
print_linked_list(linked_list* tokens)
{
    for (; tokens; tokens = tokens->tail) {
        printf("%s\n", tokens->head);
    }
}

long
length(linked_list* tokens)
{
    long count = 0;
    for (; tokens; tokens = tokens->tail) {
    	count++;
    }
    return count;
}

linked_list*
reverse(linked_list* tokens)
{
    linked_list* tokensTemp = 0;
    for (; tokens; tokens = tokens->tail) {
    	tokensTemp = cons(tokens->head, tokensTemp);
    }
    return tokensTemp;
}

linked_list*
rev_free(linked_list* tokens)
{
    linked_list* tokensTemp = reverse(tokens);
    free_linked_list(tokens);
    return tokensTemp;
}
