#ifndef TAC_H
#define TAC_H

#include "types.h"
#include "symbols.h"

#define NUM_STMT 	17
#define STMT_STR(x) 	stmt_##x
#define STMT_TABLE 			\
	/*   enum 	fmt*/		\
	STMT(label, 	"L$l"), 	\
	/*Arithmetic*/ 			\
	STMT(add, 	"$0 = $1 + $2"),\
	STMT(neg, 	"$0 = $1 - $2"),\
	STMT(mul, 	"$0 = $1 * $2"),\
	STMT(div, 	"$0 = $1 / $2"),\
	/*Binary*/ 			\
	STMT(band, 	"$0 = $1 & $2"),\
	STMT(bor, 	"|"), 		\
	STMT(bnot, 	"~"), 		\
	/*Memory*/ 			\
	STMT(load, 	"ld $0, $1"), 	\
	STMT(store, 	"st $0, $1"), 	\
	/*Relational*/ 			\
	STMT(lt, 	"$1 < $2"), 	\
	STMT(gt, 	"$1 > $2"), 	\
	STMT(eq, 	"$1 == $2"), 	\
	STMT(neq, 	"$1 != $2"), 	\
	STMT(call, 	"$1()"), 	\
	STMT(ugoto, 	"goto $l"), 	\
	STMT(cgoto, 	"cgoto $l")

#define OPER_STR(x) 	oper_##x
#define OPER_TABLE 					\
	/*   enum 	fmt 	get_function */		\
	OPER(reg, 	"%d", 	get_reg)		\
	OPER(sym, 	"%s", 	get_ident) 		\
	OPER(cnum, 	"%ld", 	get_cnum) 		\
	OPER(cstr, 	"%s", 	get_str)

/*Some operations must be transformed*/
#define STMT(a, b) STMT_STR(a)
enum stmt_type {
	STMT_TABLE
};
#undef STMT

#define OPER(a, b, c) OPER_STR(a),
enum oper_type {
	OPER_TABLE
};
#undef OPER

extern const char *stmt_str[NUM_STMT];

struct ir_operand {
	enum oper_type type;
	union {
		int virt_reg;
		struct symbol *sym;
		long constant;
		char *cstr;
	} val;
};

struct ir_stmt {
	enum stmt_type type;
	struct ir_operand *result, *arg1, *arg2;
	struct ir_stmt *prev, *next;
};

void debug_fmt(const char *fmt, struct ir_stmt *stmt);
void debug_ir_operand(struct ir_operand *oper);
void debug_ir_stmt(struct ir_stmt *stmt);
int get_reg(struct ir_operand *oper);
char *get_ident(struct ir_operand *oper);
char *get_str(struct ir_operand *oper);
long get_cnum(struct ir_operand *oper);

#endif
