#include "lexer.h"

const struct token_pair token_pairs[29] = {
	{t_int, 	"int", 	3},
	{t_char, 	"char", 4},
	{t_inc, 	"++", 	2},
	{t_dec, 	"--", 	2},
	{t_plus, 	"+", 	1},
	{t_minus, 	"-", 	1},
	{t_asterisk, 	"*", 	1},
	{t_vslash, 	"|", 	1},
	{t_ampersand, 	"&", 	1},
	{t_rslash, 	"/", 	1},
	{t_eq, 		"==", 	2},
	{t_equal, 	"=", 	1},
	{t_lparen, 	"(", 	1},
	{t_rparen, 	")", 	1},
	{t_lcbrack, 	"{", 	1},
	{t_rcbrack, 	"}", 	1},
	{t_lbrack, 	"[", 	1},
	{t_rbrack, 	"]", 	1},
	{t_gte, 	">=", 	2},
	{t_lte, 	"<=", 	2},
	{t_gt, 		">", 	1},
	{t_lt, 		"<", 	1},
	{t_neq, 	"!=", 	2},
	{t_if, 		"if", 	2},
	{t_else, 	"else", 4},
	{t_while, 	"while",5},
	{t_for, 	"for", 	3},
	{t_semic, 	";", 	1}
};

/*Splits null delimited character stream into tokens and their respective types*/
struct token **tokenify(char *stream, int *numt)
{
	if (!stream) return NULL;
	struct token ** tokens = NULL;

	int tlen = 0, numtok = 0, ln=0;
	char *save = stream, *tok = NULL, *ls = stream;
	enum token_type t;
	while ((t=lex_r(&save, &tlen, &ls, &ln))) {
		tok = malloc(tlen+1);
		memcpy(tok, save - tlen, tlen);
		tok[tlen] = 0;
		numtok++;
		struct token *token = malloc(sizeof(struct token));
		token->tok = tok, token->type = t;
		token->col =  save - ls, token->line = ln;
		if (!tokens) tokens = malloc(sizeof(struct token *));
		else tokens = realloc(tokens, sizeof(struct token *) * numtok);
		tokens[numtok-1] = token;
	}
	*numt = numtok;
	return tokens;
}

/*Given a string of arbitrary length, return the first valid token and set the length*/
enum token_type lex_r(char **saveptr, int *tlen, char **ls, int *ln)
{
	char *str = *saveptr;
	while (isspace(*str)) *ln+=*str=='\n'?(*ls=str,1):0, str++;

	enum token_type t = t_none;
	int len = 0;
	size_t ntoken_pairs = (size_t)((sizeof(token_pairs)/sizeof(struct token_pair)));
	/*Match against static tokens*/
	for (unsigned int i = 0; i < ntoken_pairs; i++) {
		if (!strncmp(str, token_pairs[i].match, token_pairs[i].len)) {
			len = token_pairs[i].len;
			t = token_pairs[i].type;
			break;
		}
	}
	/*Check for ident, string, numeric, etc*/
	if (t == t_none) {
		if (isdigit(*str)) {
			t = t_numeric;
			while (str[len] && isdigit(str[len])) len++;
		} else if (isalpha(*str) || *str == '_') {
			t = t_ident;
			while (str[len] && (isalpha(str[len]) || str[len] == '_')) len++;
		}
	}

	*saveptr = str + len;
	*tlen = len;
	return t;
}
