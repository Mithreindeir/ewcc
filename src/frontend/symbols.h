#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "types.h"
/*Because the symbol tables operate on a per scope level,
 * the small amount of average local variables just isn't worth
 * amortizing the cost of using an efficient average lookup
 * algorithm, I will just be using arrays for now.*/

/*Each architecture must provide a mechanism for allocating local variables,
 * eg: x86alloc("a") -> ebp-0x8, globals are left for linking */
struct symbol {
	unsigned long offset, size;
	char *identifier;
	struct type *type;
};

/*This table also represents a scope*/
struct symbol_table {
	unsigned long fp;
	int num_syms;
	struct symbol **syms;
	struct symbol_table *parent;
};

struct symbol_table *symbol_table_init(struct symbol_table *parent);
void symbol_table_free(struct symbol_table *scope);
void symbol_table_debug(struct symbol_table *scope);

struct type *get_type(struct symbol_table *scope, const char *ident);
void set_type(struct symbol_table *scope, char *ident, struct type *type);
int resolve_size(struct type *type);
struct symbol *get_symbol(struct symbol_table *scope, const char *ident);
unsigned alloc_type(struct symbol_table *scope, char *ident);
unsigned long get_offset(struct symbol_table *scope, char *ident);

#endif
