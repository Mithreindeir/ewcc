#ifndef DEBUG_H
#define DEBUG_H

#include "tac.h"
#include "live.h"
#include "ralloc.h"
#include "frontend/symbols.h"
#include "frontend/ast.h"

extern const char *stmt_debug_str[NUM_STMT];

/*Contains functions to dump internal structures, but that arent necessary to compile*/
void ir_debug_fmt(const char *fmt, struct ir_stmt *stmt);
void ir_operand_debug(struct ir_operand *oper);
void ir_operand_size_debug(struct ir_operand *oper);
void ir_debug(struct ir_stmt *head);
void ir_stmt_debug(struct ir_stmt *stmt);
void symbol_table_debug(struct symbol_table *scope);
void bb_debug(struct bb *head);
void node_debug(struct node *n);

#endif
