#ifndef TAC_H
#define TAC_H

#include "types.h"
#include "symbols.h"
#include "ast.h"

/*Some flag values*/
#define ADDRESS 	1
#define VALUE 		2
#define IGNORE 		3

#define CONDITIONAL 	1
#define UNCONDITIONAL 	2

#define REQ_LABEL(ctx) (ctx->label_cnt++)

#define NUM_STMT 	20
#define STMT_STR(x) 	stmt_##x
/*X Macros to generate statement mapping and automatically debug printing
 * IR tries to be solid intermediate between the AST and assembly
 * while staying architecture independent.
 * Clarifications:
 * LD: Dereferences $1 and stores it in $0
 * ST: Stores $1 at address $0
 * */
#define STMT_TABLE 				\
	/*   enum 	fmt*/			\
	STMT(invalid, 	, 	"")		\
	STMT(label, 	, 	"L$l") 		\
	/**/ 					\
	STMT(call, 	, 	"$1()") 	\
	STMT(ugoto, 	, 	"goto L$l") 	\
	STMT(cgoto, 	, 	"ifneq L$l") 	\
	STMT(alloc, 	, 	"alloc $1")
	/*Arithmetic*/
#define STMT_OPER_TABLE 			\
	STMT(add, 	o_add, 	"$0 = $1 + $2") \
	STMT(sub, 	o_sub, 	"$0 = $1 - $2") \
	STMT(mul, 	o_mul, 	"$0 = $1 * $2") \
	STMT(div, 	o_div, 	"$0 = $1 / $2") \
	/*Binary*/ 				\
	STMT(band, 	o_band, "$0 = $1 & $2") \
	STMT(bor, 	o_bor, 	"|") 		\
	STMT(bnot, 	o_bnot, "~") 		\
	/*Memory*/ 				\
	STMT(load, 	o_deref, "LD $0, [$1]")	\
	STMT(store, 	o_asn, "ST [$0], $1") 	\
	STMT(addr, 	o_ref, "$0 = &$1") 	\
	/*Relational*/ 				\
	STMT(lt, 	o_lt, 	"$1 < $2") 	\
	STMT(gt, 	o_gt, 	"$1 > $2") 	\
	STMT(eq, 	o_eq, 	"$1 == $2") 	\
	STMT(neq, 	o_neq, 	"$1 != $2")

/*Some operations must be transformed*/
#define STMT(a, b, c) STMT_STR(a),
enum stmt_type {
	STMT_TABLE
	STMT_OPER_TABLE
};
#undef STMT

enum oper_type {
	oper_reg,
	oper_sym,
	oper_cnum,
	oper_cstr,
};


struct ir_operand {
	enum oper_type type;
	union {
		int virt_reg;
		struct symbol *sym;
		char *ident;
		long constant;
		char *cstr;
	} val;
};

struct ir_stmt {
	enum stmt_type type;
	struct ir_operand *result, *arg1, *arg2;
	struct ir_stmt *prev, *next;
	int label;
};

/*Context information needed to generate TAC*/
struct generator {
	int reg_cnt;
	int label_cnt;
	struct symbol_table *scope;
	struct ir_stmt *head, *cur;
};

struct ir_stmt *ir_stmt_init();

struct ir_stmt *generate(struct node *n);
/*Generates TAC from AST node stmt*/
void generate_from_node(struct node *n, struct generator *context);
/*Generates TAC from AST expression, returns either address/value or ignores result*/
struct ir_operand *generate_operand(struct node *n, struct generator *context, int result);

/*Generation Helper functions*/
enum stmt_type map_stmt(enum operator op);
struct ir_operand *copy(struct ir_operand *oper);
void emit(struct generator *context, struct ir_stmt *stmt);
struct ir_stmt *emit_label(struct generator *context, int label);
struct ir_stmt *emit_jump(struct generator *context, int label, int conditional);

struct ir_stmt *ir_stmt_init();
struct ir_operand *from_reg(int reg);
struct ir_operand *from_cnum(long cnum);
struct ir_operand *from_ident(char *ident);
void ir_operand_free(struct ir_operand *oper);
void ir_stmt_free(struct ir_stmt *stmt);

#endif
