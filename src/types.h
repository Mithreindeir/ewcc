#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_OPERATORS 31
#define NUM_DTYPES 9

/*All operator types*/
enum operator {
	/*arithmetic*/
	o_add,
	o_sub,
	o_mul,
	o_div,
	/*relative*/
	o_eq,
	o_neq,
	o_gt,
	o_gte,
	o_lt,
	o_lte,
	/*logical*/
	o_and,
	o_or,
	o_not,
	/*binary*/
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
	/*assignment*/
	o_asn,
	o_add_asn,
	o_sub_asn,
	o_mul_asn,

	o_invalid
};


enum type_specifier {
	type_none,
	type_int,
	type_char,
	type_short,
	type_long,
	type_float,
	type_double,
	type_signed,
	type_unsigned,
};


/*Type connstructors*/
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
		int arr_elements;
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

/*Returns operator given token string, ignoring ambiguous operators (like & binary and and & ref) */
struct type *type_init(enum info_type node_type);
/*Merges two linked lists of type info, with t1 at the front, and returns it*/
struct type *types_merge(struct type *t1, struct type *t2);
void type_free(struct type *head);
void print_type(struct type *head);
int cmp_types(struct type *a, struct type *b);
struct type *dereference_type(struct type *a);
enum operator dec_oper(char *str);
void print_oper(enum operator o);

#endif