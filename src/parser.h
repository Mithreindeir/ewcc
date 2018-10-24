#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "lexer.h"
#include "ast.h"
#include "symbols.h"

#define save(p) (p->cur)
#define restore(p, save) (p->cur = save)

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
int peek(struct parser * p, enum token_type t);
void syntax_error(struct parser *p, const char *msg);
struct token *match(struct parser *p, enum token_type t);

/*Recursive Descent Parser */
struct expr *parse_primary_expr(struct parser *p);
struct expr *parse_postfix(struct parser *p);
struct expr *parse_unary(struct parser *p);

struct expr *parse_mul_expr(struct parser *p);
struct expr *parse_add_expr(struct parser *p);

struct expr *parse_rel_expr(struct parser *p);
struct expr *parse_eq_expr(struct parser *p);
struct expr *parse_and_expr(struct parser *p);
struct expr *parse_or_expr(struct parser *p);

struct expr *parse_asn_expr(struct parser *p);
struct expr *parse_expr(struct parser *p);

struct stmt *parse_expr_stmt(struct parser *p);
struct stmt *parse_block(struct parser *p);
struct stmt *parse_cond(struct parser *p);
struct stmt *parse_loop(struct parser *p);
struct stmt *parse_decl(struct parser *p);
struct stmt *parse_stmt(struct parser *p);


int parse_dir_declarator(struct parser *p, struct declaration *decl);
int parse_declarator(struct parser *p, struct declaration *decl);
struct stmt *parse_declaration(struct parser *p);
struct declaration *parse_init_declarator(struct parser *p, enum type_specifier dtype);

#endif
