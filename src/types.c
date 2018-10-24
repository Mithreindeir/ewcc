#include "types.h"

const char *operator_str[NUM_OPERATORS] = {
	"+", "-", "*", "/", 			// Arithmetic
	"==", "!=", ">", ">=", "<", "<=", 	// Relational
	"&&", "||", "!", 			// Logical
	"&", "|", "^", "~", 			// Binary
	"-", "&", "*", "++", "++", "--", "--",// Unary/postfix/ptr
	"=", "+=", "-=", "*=", "/=", 		// Assignment
	"invalid"
};

const char *data_type_str[NUM_DTYPES] = {
	"none", "int", "char", "short", "long", "float", "double", "signed", "unsigned"
};

struct type *type_init(enum info_type node_type)
{
	struct type *t = malloc(sizeof(struct type));
	t->type = node_type;
	t->next = NULL;
	return t;
}

struct type *types_merge(struct type *t1, struct type *t2)
{
	struct type *t3 = t1 ? t1 : t2;
	if (t1) {
		while (t1->next) t1 = t1->next;
		t1->next = t2;
	}
	return t3;
}

void type_free(struct type *head)
{
	if (!head) return;
	type_free(head->next);
	if (head->type == type_fcn) {
		for (int i = 0; i < head->info.fcn.paramc; i++) {
			free(head->info.fcn.paramv[i]);
			type_free(head->info.fcn.paramt[i]);
		}
		free(head->info.fcn.paramv);
		free(head->info.fcn.paramt);
	}
	free(head);
}

void print_type(struct type *head)
{
	if (!head) return;
	switch (head->type) {
		case type_datatype:
			printf("%s", data_type_str[head->info.data_type]);
			print_type(head->next);
			break;
		case type_ptr:
			printf("ptr(");
			print_type(head->next);
			printf(")");
			break;
		case type_array:
			printf("arr(");
			print_type(head->next);
			printf(")");
			break;
		case type_fcn:
			printf("fcn( ");
			for (int i = 0; i < head->info.fcn.paramc; i++) {
				printf("%s: ", head->info.fcn.paramv[i]);
				print_type(head->info.fcn.paramt[i]);
				if ((i+1) < head->info.fcn.paramc) printf(", ");
			}
			printf(") -> ");
			print_type(head->next);
			break;
	}
}


int cmp_types(struct type *a, struct type *b)
{
	if (!a || !b) return 0;
	if (a->next && b->next) return cmp_types(a->next, b->next);
	if ((!a->next && b->next) || (b->next && !a->next)) return 0;
	if (a->type != b->type) return 0;
	if (a->type == type_datatype && a->info.data_type != b->info.data_type) return 0;
	return 1;
}

struct type *dereference_type(struct type *a)
{
	return a ? a->next : NULL;
}

enum operator dec_oper(char *str)
{
	for (int i = 0; i < NUM_OPERATORS; i++) {
		if (!strcmp(str, operator_str[i])) {
			return i;
		}
	}
	return o_invalid;
}

void print_oper(enum operator o)
{
	printf("%s", operator_str[o]);
}
