#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "pcommon.h"
#include "cdecl.h"

#define save(p) (p->cur)
#define restore(p, save) (p->cur = save)

/*There may be a time when having a recursive function for each operator precedence is not
 * effective, and instead a table will be used, however the current binary operations only
 * take up ~25% of the parser*/
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
struct stmt *parse_stmt(struct parser *p);

struct stmt *parse_jump(struct parser *p);

#endif
