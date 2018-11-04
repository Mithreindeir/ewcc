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
			if (!(cexpr = parse_expr(p)))
				syntax_error(p,
					     "Missing constant expression in array decl");
			if (cexpr->type != node_cnum)
				syntax_error(p,
					     "Only constant value cexprs implemented right now");
			if (!match(p, t_rbrack))
				syntax_error(p,
					     "Missing right bracket in array declaration");
			struct type *arr;
			decl->type =
			    types_merge(decl->type,
					(arr = type_init(type_array)));
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
	int s = save(p);
	struct declaration *decl;
	decl = malloc(sizeof(struct declaration));
	decl->initializer = NULL;
	decl->ident = NULL;
	decl->type = NULL;
	if (!parse_declarator(p, decl)) {
		free(decl);
		restore(p, s);
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
	enum type_specifier dtype = type_void;
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
	enum type_specifier dtype = type_void;
	if (match(p, t_int))
		dtype = type_int;
	if (match(p, t_char))
		dtype = type_char;
	if (!dtype)
		return NULL;
	struct declaration_list *dlist=NULL;
	struct declaration *decl, **decl_list = NULL;
	int nd = 0, comma = 0, semic = 0;
	while ((decl=parse_init_declarator(p, dtype))) {
		comma = 0;
		nd++;
		if (!decl_list) decl_list = malloc(sizeof(struct declaration*));
		else decl_list = realloc(decl_list, sizeof(struct declaration*)*nd);
		set_type(get_scope(p), decl->ident, decl->type);
		if (match(p, t_equal)) {
			struct expr *e = parse_asn_expr(p);
			if (!e)
				syntax_error(p, "Invalid initializer after declaration");
			decl->initializer = e;
		}

		decl_list[nd-1] = decl;
		if (match(p, t_comma)) {
			comma = 1;
		} else if (match(p, t_semic)) {
			semic = 1;
			break;
		}
	}
	if (comma) syntax_error(p, "Expected another declaration after comma");
	if (!semic) syntax_error(p, "Missing semicolon after declaration");
	if (nd) {
		dlist = malloc(sizeof(struct declaration_list));
		dlist->num_decls = nd;
		dlist->decls = decl_list;
	}
	return node_init(node_decl_list, dlist);
}

/*F = {decl_spec}* declarator param_list block*/
struct stmt *parse_function(struct parser *p)
{
	int s = save(p);
	enum type_specifier dtype = type_int;
	if (match(p, t_void))
		dtype = type_void;
	else if (match(p, t_int))
		dtype = type_int;
	else if (match(p, t_char))
		dtype = type_char;
	else
		printf("Warning: return type defaults to int\n");
	/*Validify function declaration, then parse body */
	struct declaration *decl;
	decl = malloc(sizeof(struct declaration));
	decl->initializer = NULL;
	decl->ident = NULL;
	decl->type = NULL;
	if (!parse_declarator(p, decl)) {
		free(decl);
		restore(p, s);
		return 0;
	}
	if (!decl->type)
		syntax_error(p, "Invalid declarator");
	/*The type must be a function */
	if (decl->type->type != type_fcn) {
		free(decl->ident);
		type_free(decl->type);
		free(decl);
		restore(p, s);
		return 0;
	}

	struct type *decldtype = type_init(type_datatype);
	decldtype->info.data_type = dtype;
	decl->type = types_merge(decl->type, decldtype);
	/*Create function from declaration */
	struct stmt *block;
	if (!(block = parse_block(p))) {
		free(decl->ident);
		type_free(decl->type);
		free(decl);
		restore(p, s);
		return 0;
	}
	set_type(get_scope(p), decl->ident, decl->type);
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
