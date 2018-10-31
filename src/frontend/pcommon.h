#ifndef PCOMMON_H
#define PCOMMON_H

#include "lexer.h"
#include "ast.h"
#include "symbols.h"

/*Parser data structure and helper functiosn*/
struct parser {
	struct symbol_table **scope_stk;
	int num_stk;
	struct token **toks;
	int cur, len;
};


struct parser *parser_init(struct token **tlist, int num_tokens);
void parser_free(struct parser *p);

void push_scope(struct parser *p);
struct symbol_table *pop_scope(struct parser *p);
struct symbol_table *get_scope(struct parser *p);

struct token *consume(struct parser *p);
int peek(struct parser *p, enum token_type t);
void syntax_error(struct parser *p, const char *msg);
struct token *match(struct parser *p, enum token_type t);

#endif
