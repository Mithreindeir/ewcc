#include "types.h"

const char *operator_str[NUM_OPERATORS] = {
	"+", "-", "*", "/",	// Arithmetic
	"==", "!=", ">", ">=", "<", "<=",	// Relational
	"&&", "||", "!",	// Logical
	"&", "|", "^", "~",	// Binary
	"-", "&", "*", "++", "++", "--", "--",	// Unary/postfix/ptr
	"=", "+=", "-=", "*=", "/=",	// Assignment
	"invalid"
};

#define TYPE(a, b, c) [a]=b
const char *data_type_str[NUM_DTYPES] = {
	TYPE_TABLE
};

#undef TYPE

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
		while (t1->next)
			t1 = t1->next;
		t1->next = t2;
	}
	return t3;
}

void type_free(struct type *head)
{
	if (!head)
		return;
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

struct type *type_copy(struct type *top)
{
	if (!top) return NULL;
	struct type *cpy = type_init(top->type);
	memcpy(&cpy->info, &top->info, sizeof(top->info));
	if (top->type == type_fcn) {
		struct func_type fcn = top->info.fcn;
		char **paramv  = malloc(sizeof(char*)*fcn.paramc);
		struct type **paramt = malloc(sizeof(struct type*)*fcn.paramc);
		for (int i = 0; i < fcn.paramc; i++) {
			int slen = strlen(fcn.paramv[i]);
			char *np = malloc(slen+1);
			memcpy(np, fcn.paramv[i], slen);
			np[slen] = 0;
			paramv[i] = np;
			paramt[i] = type_copy(fcn.paramt[i]);
		}
		cpy->info.fcn.paramv = paramv;
		cpy->info.fcn.paramt = paramt;
	}
	if (top->next) cpy->next = type_copy(top->next);
	return cpy;
}

int resolve_size(struct type *type)
{
	struct type *head = type;
	int m = 1;
	while (head) {
		if (head->type == type_ptr) return PTR_SIZE*m;
		if (head->type == type_datatype) {
			switch (head->info.data_type) {
				case type_int: return INT_TYPE_SIZE*m;
				case type_char: return CHAR_TYPE_SIZE*m;
				default: break;
			}
		}
		if (head->type == type_array) m = m * head->info.elem;
		head = head->next;
	}
	/*Default to integer*/
	printf("Error resolving size of type: ");
	print_type(type);
	printf("\n");
	return 0;
}

void print_type(struct type *head)
{
	if (!head)
		return;
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
			if ((i + 1) < head->info.fcn.paramc)
				printf(", ");
		}
		printf(") -> ");
		print_type(head->next);
		break;
	}
}


int cmp_types(struct type *a, struct type *b)
{
	if (!a || !b)
		return 0;
	if (a->next && b->next)
		return cmp_types(a->next, b->next);
	if ((!a->next && b->next) || (b->next && !a->next))
		return 0;
	if (a->type != b->type)
		return 0;
	if (a->type == type_datatype
	    && a->info.data_type != b->info.data_type)
		return 0;
	return 1;
}

struct type *dereference_type(struct type *a)
{
	return a ? a->next : NULL;
}

enum operator  dec_oper(char *str)
{
	for (int i = 0; i < NUM_OPERATORS; i++) {
		if (!strcmp(str, operator_str[i])) {
			return i;
		}
	}
	return o_invalid;
}

void print_oper(enum operator  o)
{
	printf("%s", operator_str[o]);
}
