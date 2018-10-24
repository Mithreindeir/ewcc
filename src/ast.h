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
	/*Empty node for grammar*/
	node_empty,
	/*Expressions*/
	node_ident,
	node_cnum,
	node_unop,
	node_binop,
	node_fcall,
	/*Statement*/
	node_func,
	node_block,
	node_cond,
	node_loop,
	node_decl,
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

struct block {
	struct stmt **stmt_list;
	int num_stmt;
	/*Scope level symbol table*/
	struct symbol_table *scope;
};

struct loop {
	struct expr *init, *condition, *iter;
	struct stmt *body;
};

struct cond {
	struct expr *condition;
	struct stmt *body;
	struct stmt *otherwise;
};

struct binop {
	struct expr *lhs, *rhs;
	enum operator op;
};

struct unop {
	struct expr *term;
	enum operator op;
};

/*Declarations are a bitch:
 * The grammar is as follows:
 * declaration:
 *  		| { declaration-specifier }+ { init-declarator }*
 *
 * init-declarator:
 * 		| declarator
 * 		| declarator = intializer
 * 		;
 *
 * declaration-specifier:
 *  		| storage-class-specifier
 *  		| type-specifier
 *  		| type-qualifier
 *  		;
 *
 * declarator:
 * 		| {ptr}? dir_declr
 * 		;
 * direct declarator:
 * 		| ident
 *  		| ( declarator )
 *  		| direct_declarator [ const expr ]
 *  		| direct_declarator ( param list )
 *  		| direct_declarator ( {ident}* )
 *  		;
 *
 * Hence:
 * int; //legal
 * int int; //legal parsing technically, but error (two or more data types in decl-spec) so just throw error
 * const a; //legal, warning about the int default
 * //both different
 * int (*ptr)[2];
 * int   *ptr[2];
 *
 * //Functions make it worse
 * int (*func)(); //not too bad
 * int **(*func)(struct { int a; int b; }, const char *p[10]); //completely legal
 * basically, parentheses redirect the declarator to inject a pointer into any type
 * int *func(); //function declaration with return type of int*
 * int (*func)(); //function pointer declaration with return type of int
 * int * ->  func  -> empty-parameter-list;
 * int   -> *func  -> empty-parameter-list;
 *
 *
 * declaration's have a type name a list of 0 or more type qualifiers and they have an arbitrary
 * amount of indirection, array specifiers, function specifiers, identifiers, etc
 * additionally, any functions introduce this concept recursively, as well as possibly adding
 * "abstract" versions
 *
 * In practice, 90% of times this can be squashed into a struct like:
 * declaration {
 *  	type;
 * 	num_ptr;
 * 	identifier;
 * };
 * And adding recursion, as well as array specifiers allow all types to be expressed
 * declaration {
 * 	type;
 * 	num_ptr;
 * 	identifier;
 * 	array_constexpr;
 * 	*next;
 * };
 *
 * */


/*The goal is to have it possible for simple types to fit in a single struct:
 * int *a; 			//fits easily into below struct
 * int *(*a)(int [][][]b); 	//it works with like 7 layers of indirection, but why complain
 * This list should be the canonicalization of:
 * int *a; = ptr(int) a;
 * int *(*a)(int [][][]b); = ptr(int) -> ptr(func(arr(arr(arr(int))) -> b)) -> a;
 * */
/*A declaration is formed by nesting each specifier
 * int  *(*a)[2]; //pointer to array of 2 pointers to integers
 * "a -> ptr -> arr[2] -> ptr -> int"
 * This form is good, because the only operation needed to dereference/index is removing the first
 * element after the identifier, which is quant because it becomes an expression without an ident
 * *a; //arr[2]
 * arr[2] -> ptr -> int;
 * */

struct declaration {
	char *ident;
	struct type *type;
	/*for now just an expression*/
	struct expr *initializer;
};

union value {
	char *ident;
	long cnum;
	double cfloat;
	char *cstring;
};

struct node *node_init(int type, void *term);
struct expr *binop_init(enum operator op, struct expr *lhs, struct expr *rhs);
struct expr *unop_init(enum operator op, struct expr *term);
struct stmt *block_init();
void block_addstmt(struct block *b, struct stmt *s);
struct stmt *loop_init(struct expr *init, struct expr *cond, struct expr *iter, struct stmt *body);
struct stmt *cond_init(struct expr *cond, struct stmt *body, struct stmt *other);
struct expr *value_ident(const char *ident);
struct expr *value_num(const char *num);
void node_tree(struct node *n);
void node_print(struct node *n);
void node_free(struct node *n);

#endif
