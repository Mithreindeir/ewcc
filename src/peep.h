#ifndef PEEP_H
#define PEEP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "debug.h"
#include "tac.h"

/*Three Address Code Peephole optimizer.
 * Fixes things like
 * ...
 * ife L2
 * goto L0
 * L0
 * ...
 * into
 * ife L2
 *
 * or
 * LD R0, [_a]
 * LD R1, [_a]
 * R2 = R0 + R1
 *
 * into
 * LD R0, [_a]
 * R2 = R0 + R0
 * */

/*Pattern matching*/
#define STMT_PAT(type, r, a1, a2, l) {type, r, a1,a2, l }

/*The patterns define what should trigger the optimization functions
 * The numbers mean a matching pair, and empty strings ignore the arguments
 * */
#define PATTERN_TABLE 		\
	/*Removes gotos that jump to the next statement*/ \
	PATTERN(2, reduce_goto,\
		STMT_PAT(stmt_ugoto, "", "", "", "1"),\
		STMT_PAT(stmt_label, "", "", "", "1"))\
	/*Eliminates double consecutive loads*/\
	PATTERN(2, reduce_load,\
		STMT_PAT(stmt_load, "", "1", "", ""),\
		STMT_PAT(stmt_load, "", "1", "", ""))\
	/*Eliminates consecutive store/load of the same variable*/\
	PATTERN(2, reduce_store_load,\
		STMT_PAT(stmt_store, "1", "", "", ""),\
		STMT_PAT(stmt_load, "", "1", "", ""))\
	/*Flips cjump/ujump pairs to eliminate one*/\
	PATTERN(3, reduce_cgoto,\
		STMT_PAT(stmt_cgoto, "", "", "", "1"),\
		STMT_PAT(stmt_ugoto, "", "", "", ""),\
		STMT_PAT(stmt_label, "", "", "", "1"))\
	/*Reduces redundant labels*/\
	PATTERN(2, reduce_label,\
		STMT_PAT(stmt_label, "", "", "", ""),\
		STMT_PAT(stmt_label, "", "", "", ""))\

#define NUM_PSTMT 11
#define NUM_PATTERNS 5

struct stmt_pattern {
	enum stmt_type st_type;
	char *result, *arg1, *arg2, *label;
};

struct pattern {
	int num_stmt;
	struct ir_stmt *(*reduce)(struct ir_stmt *);
};

extern const struct stmt_pattern pattern_stmts[NUM_PSTMT];
extern const struct pattern peephole_patterns[NUM_PATTERNS];

struct pattern_state {
	int num_vars;
	char **names;
	void **vars;
};


void optimize(struct ir_stmt *start);
struct ir_stmt *optimize_stmt(struct ir_stmt *a);
int match_pattern(struct pattern_state *state, struct ir_stmt *a, struct stmt_pattern b);
int compare_operands(struct ir_operand *a, struct ir_operand *b);
void add_state(struct pattern_state *state, void *o, char *name);
void clear_state(struct pattern_state *state);
void *query_state(struct pattern_state *state, char *n);

struct ir_stmt *reduce_goto(struct ir_stmt *a);
struct ir_stmt *reduce_cgoto(struct ir_stmt *a);
struct ir_stmt *reduce_load(struct ir_stmt *a);
struct ir_stmt *reduce_store_load(struct ir_stmt *a);
struct ir_stmt *reduce_label(struct ir_stmt *a);


#endif
