#ifndef CHECKER_H
#define CHECKER_H

#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "symbols.h"


/*Per Scope Generic Hash Table for symbols*/

/*Traverses the AST and populates the symbol table*/
void type_error(struct node *parent, struct type *a, struct type *b);
void node_check(struct node *n, struct symbol_table *scope);


#endif
