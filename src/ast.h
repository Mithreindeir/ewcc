#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "symbols.h"

/*Implicit AST Nodes defined for clarity*/
#define stmt node
#define expr node

/*To Make it Obvious where casting is used, all casting of AST nodes must use the macros*/
#define LOOP(n) ((struct loop*)n->child)
#define BLOCK(n) ((struct block*)n->child)
#define COND(n) ((struct cond*)n->child)
#define BINOP(n) ((struct binop*)n->child)
#define UNOP(n) ((struct unop*)n->child)
#define DECL(n) ((struct declaration*)n->child)
#define FUNC(n) ((struct func*)n->child)
#define IDENT(n) (((union value*)n->child)->ident)
#define CNUM(n) (((union value*)n->child)->cnum)


/*Explicit Node Type*/
enum node_type {
	/*Empty for grammar */
	node_empty,
	/*Expressions */
	node_ident,
	node_cnum,
	node_unop,
	node_binop,
	/*Statements */
	node_func,
	node_block,
	node_cond,
	node_loop,
	node_decl
};

/*Implicit AST Node: Any structural node that conveys no information
 * the parser encounters many stages of indirection, but the tree should only retain the ones that hold info
 * Ex: "i + 10;"
 *  	stmt 		      node
 *  	 |  			|
 *    expr_stmt 	    bin oper
 *       |                      +
 *      expr 	   --->       /   \
 *    	 |                   i    10;
 *    bin oper
 *       +
 *     /   \
 *    i    10
 * My intention was to have simple, understandable trees
 * */
struct node {
	int type;
	void *child;
	/* Fields for code generator passes */
	struct node_info *inf;
};

/*Explicit AST Types
 * If the types are part of the same grammar sentence, put them in a union, otherwise individual struct,
 * unless both are necessary, then keep the struct
 * Ex:
 * primary: constant-int | identifier ...
 * union { constant-int, identifier }
 * */

/*Function definition*/
struct func {
	char *ident;
	struct type *ftype;
	struct stmt *body;
};

/*Compound statement, contains symbol table of scope*/
struct block {
	struct stmt **stmt_list;
	int num_stmt;
	/*Scope level symbol table */
	struct symbol_table *scope;
};

/*Iterative statements, for, while, do while*/
struct loop {
	struct expr *init, *condition, *iter;
	struct stmt *body;
};

/*Conditional statement, if, if/else, switch*/
struct cond {
	struct expr *condition;
	struct stmt *body;
	struct stmt *otherwise;
};

/*Binary operation*/
struct binop {
	struct expr *lhs, *rhs;
	enum operator   op;
};

/*Unary operation*/
struct unop {
	struct expr *term;
	enum operator   op;
};

struct declaration {
	char *ident;
	struct type *type;
	/*for now just an expression */
	struct expr *initializer;
};

/*Primary expression or unit value in expression*/
union value {
	char *ident;
	long cnum;
	double cfloat;
	char *cstring;
};

struct node *node_init(int type, void *term);
struct expr *binop_init(enum operator   op, struct expr *lhs,
			struct expr *rhs);
struct expr *unop_init(enum operator   op, struct expr *term);
struct stmt *block_init();
void block_addstmt(struct block *b, struct stmt *s);
struct stmt *loop_init(struct expr *init, struct expr *cond,
		       struct expr *iter, struct stmt *body);
struct stmt *cond_init(struct expr *cond, struct stmt *body,
		       struct stmt *other);
struct expr *value_ident(const char *ident);
struct expr *value_num(const char *num);

void node_tree(struct node *n);
void node_print(struct node *n);
void node_free(struct node *n);

#endif
