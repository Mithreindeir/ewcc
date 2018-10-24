#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*All the token types*/
enum token_type {
	t_none,
	t_numeric,
	t_str,
	t_int,
	t_char,
	t_ident,
	t_plus,
	t_minus,
	t_asterisk,
	t_vslash,
	t_ampersand,
	t_rslash,
	t_equal,
	t_lparen,
	t_rparen,
	t_lbrack,
	t_rbrack,
	t_lcbrack,
	t_rcbrack,
	t_if,
	t_else,
	t_eq,
	t_neq,
	t_gt,
	t_lt,
	t_gte,
	t_lte,
	t_inc,
	t_dec,
	t_while,
	t_for,
	t_comma,
	t_semic,
};

struct token_pair {
	enum token_type type;
	const char * match;
	int len;
};

/*All constant tokens are in this array*/
extern const struct token_pair token_pairs[30];

struct token {
	char *tok;
	int col, line, len;
	enum token_type type;
};

/*Splits null delimited character stream into tokens and their respective types*/
struct token **tokenify(char * stream, int * tlen);
/*Given a string of arbitrary length, return the first valid token and set the length*/
enum token_type lex_r(char **saveptr, int *tlen, char **ls, int *ln);

#endif
