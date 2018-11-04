#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#define TARGET_X86
#ifdef TARGET_X86
#include "../x86types.def"
#endif

#define NUM_OPERATORS 31
#define NUM_DTYPES 9

/*All operator types (partial list at the moment)*/
enum operator {
	/*arithmetic */
	o_add,
	o_sub,
	o_mul,
	o_div,
	/*relative */
	o_eq,
	o_neq,
	o_gt,
	o_gte,
	o_lt,
	o_lte,
	/*logical */
	o_and,
	o_or,
	o_not,
	/*binary */
	o_band,
	o_bor,
	o_bxor,
	o_bnot,
	/**/
	o_neg,
	o_ref,
	o_deref,
	o_postinc,
	o_preinc,
	o_postdec,
	o_predec,
	/*assignment */
	o_asn,
	o_add_asn,
	o_sub_asn,
	o_mul_asn,
	o_invalid
};

#define TYPE_TABLE 			\
	TYPE(type_void, "void", t_void), 	\
	TYPE(type_int, 	"int", 	t_int), 	\
	TYPE(type_char, "char", t_char), 	\
	TYPE(type_short, "short", t_short), 	\
	TYPE(type_long, "long", t_long), 	\
	TYPE(type_float, "float", t_float), 	\
	TYPE(type_double, "double", t_double), 	\
	TYPE(type_signed, "signed", t_signed), 	\
	TYPE(type_unsigned, "unsigned", t_unsigned)

#define TYPE(a, b, c) a
enum type_specifier {
	TYPE_TABLE
};
#undef TYPE

/*Type constructors*/
enum info_type {
	type_datatype,
	type_ptr,
	type_array,
	type_fcn
};

/*Functions require extra type information*/
struct func_type {
	int paramc;
	char **paramv;
	struct type **paramt;
};

/*Types are constructed by combining one or more type structures in a list
 * Example:
 * int (*p)[5];
 * p: ptr(arr[5](int));
 * */
struct type {
	union {
		enum type_specifier data_type;
		struct func_type fcn;
		int elem;
	} info;
	enum info_type type;
	struct type *next;
};

/*Node info:
 * Typeinfo: All AST nodes evaluate to a type (or void for statements)
 * Truelist/Falselist: Used for short circuit code generation
 * Virtual register: When translating to IR, value to keep track of what holds the nodes value
 * */
struct node_info {
	struct type *type;
	void *truelist, *falselist;
	int virt_reg;
};

/*Arrays for debugging enums*/
extern const char *operator_str[NUM_OPERATORS];
extern const char *data_type_str[NUM_DTYPES];

struct type *type_init(enum info_type node_type);
/*Merges two type constructor lists*/
struct type *types_merge(struct type *t1, struct type *t2);
void type_free(struct type *head);
struct type *type_copy(struct type *top);
int resolve_size(struct type *type);
void print_type(struct type *head);
int cmp_types(struct type *a, struct type *b);
struct type *dereference_type(struct type *a);
/*Returns operator enum given the token string, works for unambigious operators*/
enum operator   dec_oper(char *str);
void print_oper(enum operator   o);

#endif
