#include "pcommon.h"

struct parser *parser_init(struct token **tlist, int num_tokens)
{
	struct parser *p = malloc(sizeof(struct parser));

	p->scope_stk = NULL, p->num_stk = 0;
	push_scope(p);
	p->toks = tlist, p->len = num_tokens;
	p->cur = 0;

	return p;
}

void parser_free(struct parser *p)
{
	for (int i = 0; i < p->len; i++) {
		free(p->toks[i]->tok);
		free(p->toks[i]);
	}
	symbol_table_free(pop_scope(p));
	free(p->toks);
	free(p);
}

/*Parser helper functions*/
struct token *match(struct parser *p, enum token_type t)
{
	if (peek(p, t)) {
		return consume(p);
	}
	return NULL;
}

int peek(struct parser *p, enum token_type t)
{
	if (p->cur >= p->len)
		return 0;
	return p->toks[p->cur]->type == t;
}

struct token *consume(struct parser *p)
{
	++p->cur;
	if (p->cur > p->len) {
		return NULL;
	}
	return p->toks[p->cur - 1];
}

void syntax_error(struct parser *p, const char *msg)
{
	if (p->cur >= p->len)
		p->cur = p->len - 1;
	struct token *t = p->toks[p->cur];
	fprintf(stderr, "\n(%d,%d): error \"%s\" Near Token %s\n", t->col,
		t->line, msg, t->tok);
	exit(1);
}

void push_scope(struct parser *p)
{
	struct symbol_table *top = get_scope(p);
	p->num_stk++;
	if (!p->scope_stk)
		p->scope_stk = malloc(sizeof(struct symbol_table *));
	else
		p->scope_stk =
		    realloc(p->scope_stk,
			    sizeof(struct symbol_table *) * p->num_stk);
	p->scope_stk[p->num_stk - 1] = symbol_table_init(top);
}

struct symbol_table *pop_scope(struct parser *p)
{
	if (!p->num_stk)
		return NULL;
	struct symbol_table *s = p->scope_stk[p->num_stk - 1];
	p->num_stk--;
	if (!p->num_stk) {
		free(p->scope_stk);
		p->scope_stk = NULL;
	} else {
		p->scope_stk =
		    realloc(p->scope_stk,
			    sizeof(struct symbol_table *) * p->num_stk);
	}
	return s;
}

struct symbol_table *get_scope(struct parser *p)
{
	return p->num_stk ? p->scope_stk[p->num_stk - 1] : NULL;
}

