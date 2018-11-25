#ifndef PEEP_H
#define PEEP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "match.h"
#include "debug.h"
#include "tac.h"

/*Three Address Code Peephole optimizer*/

#define RED_GOTO 	0
#define RED_CGOTO 	1
#define RED_LOAD 	2
#define RED_STLOAD 	3
#define RED_LABEL 	4
#define UNUSED_LABEL 	5
#define DOUBLE_GOTO 	6
#define INV_MOVE 	7

/*The patterns define what should trigger the optimization functions
 * The numbers mean a matching pair, and empty strings ignore the arguments
 * */
#define PATTERN_TABLE 		\
	PATTERN(2, RED_GOTO,\
		STMT_PAT(SRC | LBL, 	stmt_ugoto, IGN, IGN, IGN),\
		STMT_PAT(DST | LBL, 	stmt_label, IGN, IGN, IGN))\
	PATTERN(2, RED_STLOAD,\
		STMT_PAT(SRC, 		stmt_store, IGN, AG1, IGN),\
		STMT_PAT(DST, 		stmt_load, IGN, IGN, IGN))\
	PATTERN(3, RED_CGOTO,\
		STMT_PAT(LBL | SRC, 	stmt_cgoto, IGN, IGN, IGN),\
		STMT_PAT(LBL, 		stmt_ugoto, IGN, IGN, IGN),\
		STMT_PAT(LBL | DST, 	stmt_label, IGN, IGN, IGN))\
	PATTERN(2, RED_LABEL,\
		STMT_PAT(0, 		stmt_label, IGN, IGN, IGN),\
		STMT_PAT(0, 		stmt_label, IGN, IGN, IGN))\
	PATTERN(2, RED_LOAD,\
		STMT_PAT(SRC, 		stmt_load, IGN, AG1, IGN),\
		STMT_PAT(DST, 		stmt_load, IGN, IGN, IGN))\
	PATTERN(1, UNUSED_LABEL,\
		STMT_PAT(0, 		stmt_label, IGN, IGN, IGN))\
	PATTERN(2, DOUBLE_GOTO,\
		STMT_PAT(0, 		stmt_ugoto, IGN, IGN, IGN),\
		STMT_PAT(0, 		stmt_ugoto, IGN, IGN, IGN))\
	PATTERN(1, INV_MOVE,\
		STMT_PAT(0, 		stmt_move, REG, REG, IGN))

#define NUM_PSTMT 15
#define NUM_PATTERNS 8

extern struct stmt_pattern pattern_stmts[NUM_PSTMT];
extern struct pattern peephole_patterns[NUM_PATTERNS];
extern struct ir_stmt *(*reducing[NUM_PATTERNS]) (struct ir_stmt * a);


void optimize(struct ir_stmt *start);
struct ir_stmt *find_handle(struct ir_stmt *cur, int ns, int val);

struct ir_stmt *reduce_goto(struct ir_stmt *a);
struct ir_stmt *reduce_cgoto(struct ir_stmt *a);
struct ir_stmt *reduce_load(struct ir_stmt *a);
struct ir_stmt *reduce_store_load(struct ir_stmt *a);
struct ir_stmt *reduce_label(struct ir_stmt *a);
struct ir_stmt *reduce_unused_label(struct ir_stmt *a);
struct ir_stmt *reduce_double_goto(struct ir_stmt *a);
struct ir_stmt *reduce_invalid_move(struct ir_stmt *a);

#endif
