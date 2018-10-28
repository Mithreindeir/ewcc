#ifndef PEEP_H
#define PEEP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "match.h"
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
 * or
 * ife L2
 * goto L0
 * L2:
 * into
 * ifne L0
 * */

/*Pattern matching*/
#define STMT_PAT(flag, type, r, a1, a2) {flag, type, r, a1, a2}

/*The patterns define what should trigger the optimization functions
 * The numbers mean a matching pair, and empty strings ignore the arguments
 * */
#define PATTERN_TABLE 		\
	PATTERN(2, reduce_goto,\
		STMT_PAT(SRC | LBL, 	stmt_ugoto, IGN, IGN, IGN),\
		STMT_PAT(DST | LBL, 	stmt_label, IGN, IGN, IGN))\
	PATTERN(2, reduce_store_load,\
		STMT_PAT(SRC, 		stmt_store, AG1, IGN, IGN),\
		STMT_PAT(DST, 		stmt_load, IGN, IGN, IGN))\
	PATTERN(3, reduce_cgoto,\
		STMT_PAT(LBL | SRC, 	stmt_cgoto, IGN, IGN, IGN),\
		STMT_PAT(LBL, 		stmt_ugoto, IGN, IGN, IGN),\
		STMT_PAT(LBL | DST, 	stmt_label, IGN, IGN, IGN))\
	PATTERN(2, reduce_label,\
		STMT_PAT(0, 		stmt_label, IGN, IGN, IGN),\
		STMT_PAT(0, 		stmt_label, IGN, IGN, IGN))\
	PATTERN(2, reduce_load,\
		STMT_PAT(SRC, 		stmt_load, IGN, AG1, IGN),\
		STMT_PAT(DST, 		stmt_load, IGN, IGN, IGN))
#define NUM_PSTMT 11
#define NUM_PATTERNS 5

extern struct stmt_pattern pattern_stmts[NUM_PSTMT];
extern struct pattern peephole_patterns[NUM_PATTERNS];

void optimize(struct ir_stmt *start);
struct ir_stmt *find_handle(struct ir_stmt *cur, void *val);

struct ir_stmt *reduce_goto(struct ir_stmt *a);
struct ir_stmt *reduce_cgoto(struct ir_stmt *a);
struct ir_stmt *reduce_load(struct ir_stmt *a);
struct ir_stmt *reduce_store_load(struct ir_stmt *a);
struct ir_stmt *reduce_label(struct ir_stmt *a);

#endif
