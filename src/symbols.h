#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "types.h"

/*Because the symbol tables operate on a per scope level,
 * the small amount of average local variables just isn't worth
 * amortizing the cost of using an efficient average case
 * algorithm, I will just be using arrays for now.*/

/**/
struct symbol {
	char *identifier;
	struct type * type;
};

struct symbol_table {
	int num_syms;
	struct symbol **syms;
	struct symbol_table *parent;
};

struct symbol_table *symbol_table_init(struct symbol_table *parent);
void symbol_table_free(struct symbol_table *scope);
void symbol_table_debug(struct symbol_table *scope);

struct type *get_type(struct symbol_table *scope, const char *ident);
void set_type(struct symbol_table *scope, char *ident, struct type *type);

#endif
