#include "cdecl.h"

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
			struct expr *cexpr;
			if (!(cexpr=parse_expr(p)))
				syntax_error(p, "Missing constant expression in array decl");
			if (cexpr->type != node_cnum)
				syntax_error(p, "Only constant value cexprs implemented right now");
			if (!match(p, t_rbrack))
				syntax_error(p,
					     "Missing right bracket in array declaration");
			struct type *arr;
			decl->type =
			    types_merge(decl->type, (arr=type_init(type_array)));
			arr->info.elem = CNUM(cexpr);
			node_free(cexpr);
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
	struct stmt *fn = func_init(decl->ident, decl->type, block);
	free(decl);
	struct type *ftype = FUNC(fn)->ftype;
	/*lastly, add parameters to block scope */
	for (int i = 0; i < ftype->info.fcn.paramc; i++) {
		char *pn = ftype->info.fcn.paramv[i];
		struct type *pt = ftype->info.fcn.paramt[i];
		set_type(BLOCK(FUNC(fn)->body)->scope, pn, pt);
	}

	return fn;
}

