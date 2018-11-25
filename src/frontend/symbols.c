#include "symbols.h"

struct symbol_table *symbol_table_init(struct symbol_table *parent)
{
	struct symbol_table *table = malloc(sizeof(struct symbol_table));

	table->parent = parent;
	table->syms = NULL, table->num_syms = 0;
	table->fp = parent ? parent->fp : 0;

	return table;
}

void symbol_table_free(struct symbol_table *scope)
{
	if (!scope)
		return;

	for (int i = 0; i < scope->num_syms; i++) {
		free(scope->syms[i]->identifier);
		free(scope->syms[i]);
	}
	free(scope->syms);
	free(scope);
}

struct symbol *get_symbol(struct symbol_table *scope, const char *ident)
{
	return get_symbol_allocd(scope, ident, 0);
}

struct symbol *get_symbol_allocd(struct symbol_table *scope, const char *ident, int allocd)
{
	struct symbol_table *p = scope;
	struct symbol *sym = NULL;
	while (p && !sym) {
		for (int i = 0; i < p->num_syms; i++) {
			if (allocd && !p->syms[i]->allocd) continue;
			if (!strcmp(p->syms[i]->identifier, ident)) {
				sym = p->syms[i];
				break;
			}
		}
		p = p->parent;
	}

	return sym;
}

struct type *get_type(struct symbol_table *scope, const char *ident)
{
	if (!scope)
		return NULL;
	struct symbol *sym = get_symbol(scope, ident);
	return sym ? sym->type : NULL;
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

	s->offset = 0;
	int len = strlen(ident);
	s->identifier = malloc(len + 1);
	memcpy(s->identifier, ident, len);
	s->identifier[len] = 0;
	s->type = type;
	s->size = resolve_size(type);
	s->allocd = 0;
	s->iter = 0;
	scope->syms[scope->num_syms - 1] = s;
}


unsigned alloc_type(struct symbol_table *scope, char *ident)
{
	struct symbol *sym = get_symbol(scope, ident);
	if (!sym)
		return 0;
	/*If parent's frame pointer is greater than the current frame pointer, update it */
	if (scope->parent && scope->parent->fp > scope->fp)
		scope->fp = scope->parent->fp;
	/*For now, default to integer */
	scope->fp += sym->size;
	sym->offset = scope->fp;
	sym->allocd = 1;
	sym->spilled = 0;
	return sym->offset;
}

unsigned long get_offset(struct symbol_table *scope, char *ident)
{
	struct symbol *sym = get_symbol(scope, ident);
	if (!sym)
		return 0;
	return sym->offset;
}
