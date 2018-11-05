#include "parser.h"

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
	       || (t = match(p, t_lbrack)) || (t = match(p, t_lparen))) {
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
		} else if (t->type == t_lparen) {
			struct expr *call = call_init(e, NULL, 0);
			struct expr *arg;
			while ((arg = parse_asn_expr(p))) {
				call_addarg(CALL(call), arg);
				if (!match(p, t_comma))
					break;
			}
			if (!match(p, t_rparen))
				syntax_error(p,
					     "Missing right parentheses in function call");
			e = call;
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
			init = parse_expr_stmt(p);
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

/*J = break | continue | return E*/
struct stmt *parse_jump(struct parser *p)
{
	struct token *t;
	if ((t = match(p, t_break)) && match(p, t_semic))
		return node_init(node_break, NULL);
	else if (t)
		syntax_error(p, "Missing semicolon after break");

	if ((t = match(p, t_continue)) && match(p, t_semic))
		return node_init(node_continue, NULL);
	else if (t)
		syntax_error(p, "Missing semicolon after continue");

	struct expr *e;
	if (match(p, t_return)) {
		e = parse_expr(p);
		if (!match(p, t_semic))
			syntax_error(p, "Missing semicolon after return");
		return node_init(node_return, e);
	}
	return NULL;
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
	if ((s = parse_jump(p)))
		return s;
	if (match(p, t_semic))
		return node_init(node_empty, NULL);

	return NULL;
}

struct node *parse_unit(struct parser *p)
{
	struct node *unit = NULL;
	struct stmt *s;
	while ((s = parse_function(p)) || (s = parse_declaration(p))) {
		if (!unit)
			unit = unit_init();
		unit_add(UNIT(unit), s);
	}
	return unit;
}

#undef stmt
