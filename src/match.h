#ifndef MATCH_H
#define MATCH_H

/*Common Functionality for TAC pattern matching used by
 * the peephole optimizer, as well as in the instruction
 * selection phase
 * */
#include "tac.h"

/*Some simplification macros*/
#define IGN opat_ignore
#define RES opat_result
#define AG1 opat_arg1
#define AG2 opat_arg2
#define REG opat_reg
#define IMM opat_imm
#define MEM opat_mem


/*Flags for Pattern Matching*/
/*Source/destination for comparisons*/
#define SRC 	1
#define DST 	2
#define LBL 	4

/*Pattern matching*/
#define STMT_PAT(flag, type, r, a1, a2) {flag, type, r, a1, a2}

/*To match a pattern*/
enum oper_pattern {
	opat_ignore,
	opat_imm,
	opat_reg,
	opat_mem,
	opat_result,
	opat_arg1,
	opat_arg2
};

/*Constant statement type to match, and flexible operand patterns*/
struct stmt_pattern {
	/*Flags as modifiers to the patterns to simplify compare logic */
	unsigned int flags;
	enum stmt_type st_type;
	enum oper_pattern result, arg1, arg2;
};

/*A pattern can have multiple stmt_patterns attached, and a value*/
struct pattern {
	/*Pattern can have multiple statement patterns attached */
	int num_stmt;
	/*Array indice of user data*/
	int value;
};

/*Input to the find function*/
struct pattern_state {
	struct ir_stmt *entry;
	struct ir_stmt *(*callback) (struct ir_stmt * cur, int ns,
				     int val);

	struct pattern *plist;
	struct stmt_pattern *splist;
	int num_patts, num_spatts;
};

struct ir_stmt *find_stmt_match(struct pattern_state *state);
int match_pattern(struct stmt_pattern *spat, int max, struct ir_stmt *st);
int match_operand(struct ir_stmt *st, struct ir_operand *a,
		  enum oper_pattern pat);
int compare_operands(struct ir_operand *a, struct ir_operand *b);

#endif
