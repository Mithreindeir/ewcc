#ifndef CHECKER_H
#define CHECKER_H

#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "symbols.h"
#include "../debug.h"

#define LHS 0
#define RHS 1
#define NEUTRAL 3

/*The semantic checker is responsible for giving every AST expression a type,
 * as well as resolving some syntactic sugar*/

/*Callback function in the array for each AST Type*/
extern void (*ast_cb[])(struct node ** n, struct symbol_table * scope);


/*Integer rank*/
int rank(struct type *a);
/*Traverses the AST and populates the symbol table*/
/*If A can be implicitly cast to B, return AST cast node, otherwise NULL*/
int implicit_cast(struct node **a, struct node **b, int favor);
/*Forces cast from A to B*/
int explicit_cast(struct type *a, struct node **b);
/*Prints type errors and node info*/
void type_error(struct node *parent, struct type *a, struct type *b);
/*Check the AST for type errors, and out of scope/undeclared variables*/
void node_check(struct node **overwrite, struct symbol_table *scope);


#endif
