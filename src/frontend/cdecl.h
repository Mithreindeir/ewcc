#ifndef CDECL_H
#define CDECL_H

#include <stdio.h>
#include "pcommon.h"
#include "parser.h"

/*C declarations are complicated enough that I thought they should be separated from the main parser*/

struct stmt *parse_decl(struct parser *p);
struct stmt *parse_function(struct parser *p);
struct declaration *parse_param(struct parser *p);
int parse_dir_declarator(struct parser *p, struct declaration *decl);
int parse_declarator(struct parser *p, struct declaration *decl);
struct stmt *parse_declaration(struct parser *p);
struct declaration *parse_init_declarator(struct parser *p,
					  enum type_specifier dtype);

#endif
