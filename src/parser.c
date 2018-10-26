#include "parser.h"

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

/*P = ident | int | ( expr )*/
struct expr *parse_primary(struct parser *p)
{
	struct token *t;
	if ((t = match(p, t_ident)))
		return value_ident(t->tok);
	if ((t = match(p, t_numeric)))
		return value_num(t->tok);
	if (match(p, t_lparen)) {
		struct expr *e;
		if ((e = parse_expr(p))) {
			if (!match(p, t_rparen))
				syntax_error(p,
					     "Missing right parentheses");
			return e;
		}
	}
	return NULL;
}

/*P = primary | P { expr } | P++ | P-- | P ( expr_list )
 * without left rec: P = primary { ++ | -- | (expr_list ) | { expr } }
 * */
struct expr *parse_postfix(struct parser *p)
{
	struct token *t;
	struct expr *e;
	if (!(e = parse_primary(p)))
		return NULL;
	while ((t = match(p, t_inc)) || (t = match(p, t_dec))
	       || (t = match(p, t_lbrack))) {
		if (t->type == t_lbrack) {
			struct expr *sub;
			if (!(sub = parse_expr(p)))
				syntax_error(p,
					     "Invalid array subscript expression");
			if (!match(p, t_rbrack))
				syntax_error(p,
					     "Missing right bracket in subscript expression");
			e = binop_init(o_add, e, sub);
			e = unop_init(o_deref, e);
		} else {
			e = unop_init(t->type ==
				      t_inc ? o_postinc : o_postdec, e);
		}
	}

	return e;
}

/*U = postfix | ++ U | -- U | UNOP U*/
struct expr *parse_unary(struct parser *p)
{
	struct token *t;
	struct expr *u;
	if ((u = parse_postfix(p)))
		return u;
	/*Ambiguous operators, so specify types manually */
	if ((t = match(p, t_inc)) || (t = match(p, t_dec))) {
		if ((u = parse_unary(p))) {
			return unop_init(t->type ==
					 t_inc ? o_preinc : o_predec, u);
		}
		syntax_error(p, "No unary expression after prefix");
	}
	if ((t = match(p, t_asterisk)) || (t = match(p, t_ampersand))) {
		if ((u = parse_unary(p))) {
			return unop_init(t->type ==
					 t_asterisk ? o_deref : o_ref, u);
		}
	}

	return NULL;
}

/*EBNF: M = U { * U } */
struct expr *parse_mul_expr(struct parser *p)
{
	struct expr *v = parse_unary(p);
	if (!v)
		return NULL;
	struct token *t;
	while ((t = match(p, t_asterisk)) || (t = match(p, t_rslash))) {
		struct expr *v2 = parse_unary(p);
		if (!v2)
			syntax_error(p, "Unexpected token in expression");
		v = binop_init(dec_oper(t->tok), v, v2);
	}
	return v;
}

/*EBNF: A = M {+ M}*/
struct expr *parse_add_expr(struct parser *p)
{
	struct expr *v = parse_mul_expr(p);
	if (!v)
		return NULL;
	struct token *t;
	while ((t = match(p, t_plus)) || (t = match(p, t_minus))) {
		struct expr *v2 = parse_mul_expr(p);
		if (!v2)
			syntax_error(p, "Unexpected token in expression");
		v = binop_init(dec_oper(t->tok), v, v2);
	}
	return v;
}


/*R = A > A | A < A | A >= A | A <= A*/
struct expr *parse_rel_expr(struct parser *p)
{
	struct expr *v = parse_add_expr(p);
	if (!v)
		return NULL;
	struct token *t;
	while ((t = match(p, t_gt)) || (t = match(p, t_lt))
	       || (t = match(p, t_lte)) || (t = match(p, t_gte))) {
		struct expr *v2 = parse_add_expr(p);
		if (!v2)
			syntax_error(p, "Unexpected token in expression");
		v = binop_init(dec_oper(t->tok), v, v2);
	}
	return v;
}

/*E = R == R | R != R*/
struct expr *parse_eq_expr(struct parser *p)
{
	struct expr *v = parse_rel_expr(p);
	if (!v)
		return NULL;
	struct token *t;
	while ((t = match(p, t_eq)) || (t = match(p, t_neq))) {
		struct expr *v2 = parse_rel_expr(p);
		if (!v2)
			syntax_error(p, "Unexpected token in expression");
		v = binop_init(dec_oper(t->tok), v, v2);
	}
	return v;
}

struct expr *parse_and_expr(struct parser *p)
{
	struct expr *v = parse_eq_expr(p);
	if (!v)
		return NULL;
	while (match(p, t_ampersand) && match(p, t_ampersand)) {
		struct expr *v2 = parse_eq_expr(p);
		if (!v2)
			syntax_error(p, "Unexpected token in expression");
		v = binop_init(o_and, v, v2);
	}
	return v;
}

struct expr *parse_or_expr(struct parser *p)
{
	struct expr *v = parse_and_expr(p);
	if (!v)
		return NULL;
	while (match(p, t_vslash) && match(p, t_vslash)) {
		struct expr *v2 = parse_and_expr(p);
		if (!v2)
			syntax_error(p, "Unexpected token in expression");
		v = binop_init(o_or, v, v2);
	}
	return v;
}

/*A=empty | cond_expr | unary '=' asn_expr */
struct expr *parse_asn_expr(struct parser *p)
{
	struct expr *v;
	struct token *t;
	int s = save(p);
	if ((v = parse_unary(p))) {
		if ((t = match(p, t_equal))) {
			struct expr *rhs;
			if ((rhs = parse_asn_expr(p))) {
				return binop_init(dec_oper(t->tok), v,
						  rhs);
			}
			syntax_error(p,
				     "Missing right hand side in assignment operation");
		}
		node_free(v);
	}
	restore(p, s);
	if ((v = parse_or_expr(p)))
		return v;
	return NULL;
}

/*E=A*/
struct expr *parse_expr(struct parser *p)
{
	struct expr *e;
	if ((e = parse_asn_expr(p)))
		return e;
	return NULL;
}

/*S = ; | E;*/
struct stmt *parse_expr_stmt(struct parser *p)
{
	struct expr *e;
	if ((e = parse_expr(p))) {
		if (!match(p, t_semic))
			syntax_error(p, "Expected ';' after expression");
		return e;
	}
	return NULL;
}

/*B = {} | { stmt_list }
 * Although technically:
 * B = {} | { block_item_list }
 * block_item_list
 * block_item = declaration | statement
 * */
struct stmt *parse_block(struct parser *p)
{
	if (match(p, t_lcbrack)) {
		struct stmt *block = block_init();
		push_scope(p);
		struct stmt *s;
		while ((s = parse_stmt(p)) || (s = parse_declaration(p)))
			block_addstmt(block->child, s);
		if (!match(p, t_rcbrack))
			syntax_error(p, "Expected '}'");
		((struct block *) block->child)->scope = pop_scope(p);
		return block;
	}
	return NULL;
}

/*L = while ( expr ) stmt | do stmt while ( expr ) ; | for ( expr ? ; expr ? ; expr ? ;) stmt*/
struct stmt *parse_loop(struct parser *p)
{
	struct token *t;
	struct expr *init, *cond, *iter;
	struct stmt *body;
	if ((t = match(p, t_while))) {
		if (!match(p, t_lparen))
			syntax_error(p, "Missing left parentheses");
		if (!(cond = parse_expr(p)))
			syntax_error(p, "No valid expression");
		if (!match(p, t_rparen))
			syntax_error(p, "Missing right parentheses");
		if (!(body = parse_stmt(p)))
			syntax_error(p,
				     "Missing statement after while expression");
		return loop_init(NULL, cond, NULL, body);
	}
	if ((t = match(p, t_for))) {
		if (!match(p, t_lparen))
			syntax_error(p, "Missing left parentheses");
		init = parse_declaration(p);
		if (!init)
			syntax_error(p,
				     "No valid initializer statement in for loop");
		cond = parse_expr(p);
		if (!match(p, t_semic))
			syntax_error(p,
				     "No valid condition statement in for loop");
		iter = parse_expr(p);
		if (!match(p, t_rparen))
			syntax_error(p, "Missing right parentheses");
		if (!(body = parse_stmt(p)))
			syntax_error(p, "Missing statement");
		return loop_init(init, cond, iter, body);
	}

	return NULL;
}

/*C = if ( expr ) stmt else stmt | if ( expr ) stmt*/
struct stmt *parse_cond(struct parser *p)
{
	struct expr *c;
	struct token *tif;
	if ((tif = match(p, t_if)) && match(p, t_lparen)) {
		if (!(c = parse_expr(p)))
			syntax_error(p, "Expected expression after '('");
		if (match(p, t_rparen)) {
			struct stmt *s, *s2;
			if ((s = parse_stmt(p))) {
				if (match(p, t_else)) {
					if ((s2 = parse_stmt(p)))
						return cond_init(c, s, s2);
				}
				return cond_init(c, s, NULL);
			}
			syntax_error(p, "No valid statement following if");
		}
		syntax_error(p, "Missing right parentheses");
	} else if (tif) {
		syntax_error(p, "Missing left parentheses");
	}

	return NULL;
}

 /* dir_decl: {ident | ( declarator ) } { [ cexpr ] | (param list) | ( {ident}* ) }*
  * */
int parse_dir_declarator(struct parser *p, struct declaration *decl)
{
	/*Identifier */
	struct token *t;
	if ((t = match(p, t_ident))) {
		if (decl->ident)
			syntax_error(p,
				     "Too many identifiers in declaration");
		int len = strlen(t->tok);
		decl->ident = malloc(len + 1);
		memcpy(decl->ident, t->tok, len);
		decl->ident[len] = 0;
	} else if (match(p, t_lparen) && parse_declarator(p, decl)) {
		if (!match(p, t_rparen))
			syntax_error(p,
				     "Missing right parentheses in declarator");
	} else {
		return 0;
	}
	while ((t = match(p, t_lbrack)) || (t = match(p, t_lparen))) {
		if (t->type == t_lbrack) {
			if (!match(p, t_rbrack))
				syntax_error(p,
					     "Missing right bracket in array declaration");
			decl->type =
			    types_merge(decl->type, type_init(type_array));
		} else if (t->type == t_lparen) {
			/*Declaration list */
			int paramc = 0;
			char **paramv = NULL;
			struct type **paramt = NULL;
			struct declaration *d;
			while ((d = parse_param(p))) {
				paramc++;
				if (paramc == 1) {
					paramv = malloc(sizeof(char *));
					paramt =
					    malloc(sizeof(struct type *));
				} else {
					paramv =
					    realloc(paramv,
						    sizeof(char *) *
						    paramc);
					paramt =
					    realloc(paramt,
						    sizeof(struct type *) *
						    paramc);
				}
				paramv[paramc - 1] = d->ident;
				paramt[paramc - 1] = d->type;
				free(d);
				if (!match(p, t_comma))
					break;
			}
			if (!match(p, t_rparen))
				syntax_error(p,
					     "Missing right parentheses in function ptr declaration");
			struct type *fcn;
			decl->type =
			    types_merge(decl->type,
					(fcn = type_init(type_fcn)));
			fcn->info.fcn.paramc = paramc;
			fcn->info.fcn.paramv = paramv;
			fcn->info.fcn.paramt = paramt;
		}
	}
	return 1;
}


int parse_declarator(struct parser *p, struct declaration *decl)
{
	struct type *t1 = NULL;
	while (match(p, t_asterisk))
		t1 = types_merge(t1, type_init(type_ptr));
	if (parse_dir_declarator(p, decl)) {
		decl->type = types_merge(decl->type, t1);
		return 1;
	}
	return 0;
}

struct declaration *parse_init_declarator(struct parser *p,
					  enum type_specifier dtype)
{
	struct declaration *decl;
	decl = malloc(sizeof(struct declaration));
	decl->initializer = NULL;
	decl->ident = NULL;
	decl->type = NULL;
	if (!parse_declarator(p, decl)) {
		free(decl);
		return 0;
	}

	struct type *decldtype = type_init(type_datatype);
	decldtype->info.data_type = dtype;
	decl->type = types_merge(decl->type, decldtype);
	return decl;
}

/*Parameters are slightly modified declarations*/
struct declaration *parse_param(struct parser *p)
{
	enum type_specifier dtype = type_none;
	if (match(p, t_int))
		dtype = type_int;
	if (match(p, t_char))
		dtype = type_char;
	if (!dtype)
		return NULL;	/*technically wrong, types aren't required by the grammar */
	struct declaration *decl = parse_init_declarator(p, dtype);
	return decl;
}

struct stmt *parse_declaration(struct parser *p)
{
	enum type_specifier dtype = type_none;
	if (match(p, t_int))
		dtype = type_int;
	if (match(p, t_char))
		dtype = type_char;
	if (!dtype)
		return NULL;	/*technically wrong, types aren't required by the grammar */
	struct declaration *decl = parse_init_declarator(p, dtype);
	set_type(get_scope(p), decl->ident, decl->type);
	if (match(p, t_semic))
		return node_init(node_decl, decl);
	if (!match(p, t_equal))
		syntax_error(p,
			     "Expected assignment expression after declaration");
	struct expr *e = parse_asn_expr(p);
	if (!e)
		syntax_error(p, "Invalid initializer after declaration");
	decl->initializer = e;
	if (!match(p, t_semic))
		syntax_error(p, "Expected ';' after initializer");
	return node_init(node_decl, decl);
}

/*F = {decl_spec}* declarator param_list block*/
struct stmt *parse_function(struct parser *p)
{
	enum type_specifier dtype = type_none;
	if (match(p, t_int))
		dtype = type_int;
	if (match(p, t_char))
		dtype = type_char;
	if (!dtype)
		return NULL;	/*technically wrong, types aren't required by the grammar */
	/*Validify function declaration, then parse body */
	struct declaration *decl;
	decl = malloc(sizeof(struct declaration));
	decl->initializer = NULL;
	decl->ident = NULL;
	decl->type = NULL;
	if (!parse_declarator(p, decl)) {
		free(decl);
		return 0;
	}
	if (!decl->type) syntax_error(p, "Invalid declarator");
	/*The type must be a function */
	if (decl->type->type != type_fcn) {
		free(decl->ident);
		type_free(decl->type);
		free(decl);
		return 0;
	}

	struct type *decldtype = type_init(type_datatype);
	decldtype->info.data_type = dtype;
	decl->type = types_merge(decl->type, decldtype);
	set_type(get_scope(p), decl->ident, decl->type);
	/*Create function from declaration */
	struct stmt *block;
	if (!(block = parse_block(p))) {
		free(decl->ident);
		type_free(decl->type);
		free(decl);
		return 0;
	}
	struct func *f = malloc(sizeof(struct func));
	f->ident = decl->ident;
	f->ftype = decl->type;
	f->body = block;
	free(decl);
	/*lastly, add parameters to block scope */
	for (int i = 0; i < f->ftype->info.fcn.paramc; i++) {
		char *pn = f->ftype->info.fcn.paramv[i];
		struct type *pt = f->ftype->info.fcn.paramt[i];
		set_type(BLOCK(f->body)->scope, pn, pt);
	}

	return node_init(node_func, f);
}

/*S = block  | expr ; | cond | iter | jmp */
struct stmt *parse_stmt(struct parser *p)
{
	struct stmt *s;
	if ((s = parse_block(p)))
		return s;
	if ((s = parse_expr_stmt(p)))
		return s;
	if ((s = parse_cond(p)))
		return s;
	if ((s = parse_loop(p)))
		return s;
	//if (s=parse_declaration(p)) return s;
	if (match(p, t_semic))
		return node_init(node_empty, NULL);

	return NULL;
}

#undef stmt
