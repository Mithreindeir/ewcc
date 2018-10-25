#include "symbols.h"

struct symbol_table *symbol_table_init(struct symbol_table *parent)
{
	struct symbol_table *table = malloc(sizeof(struct symbol_table));

	table->parent = parent;
	table->syms = NULL, table->num_syms = 0;

	return table;
}

void symbol_table_free(struct symbol_table *scope)
{
	if (!scope)
		return;

	for (int i = 0; i < scope->num_syms; i++) {
		free(scope->syms[i]);
	}
	free(scope->syms);
	free(scope);
}

struct type *get_type(struct symbol_table *scope, const char *ident)
{
	if (!scope)
		return NULL;
	for (int i = 0; i < scope->num_syms; i++) {
		if (!strcmp(scope->syms[i]->identifier, ident))
			return scope->syms[i]->type;
	}
	return get_type(scope->parent, ident);
}

void set_type(struct symbol_table *scope, char *ident, struct type *type)
{
	scope->num_syms++;
	if (!scope->syms)
		scope->syms = malloc(sizeof(struct symbol *));
	else
		scope->syms =
		    realloc(scope->syms,
			    sizeof(struct symbol *) * scope->num_syms);
	struct symbol *s = malloc(sizeof(struct symbol));

	s->identifier = ident;
	s->type = type;
	scope->syms[scope->num_syms - 1] = s;
}
