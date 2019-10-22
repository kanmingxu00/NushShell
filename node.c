#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "node.h"

void
free_nodes(node *ptr)
{
    if (ptr) {
		if (ptr->count == 0) {
			free(ptr->op);
			free(ptr->command);
			free(ptr);
    	} else if (ptr->count == 1) {
    		free_nodes(ptr->tok_one);
			free(ptr->op);
			free(ptr);
    	} else if (ptr->count == 2) {
			free_nodes(ptr->tok_one);
			free_nodes(ptr->tok_two);
			free(ptr->op);
			free(ptr);
    	}
    }
}
