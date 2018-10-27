#include <stdio.h>
#include <unistd.h>

#include "lexer.h"
#include "parser.h"
#include "checker.h"
#include "debug.h"
#include "tac.h"
#include "peep.h"


int main()
{
	char *str = NULL;
	int slen = 1;
	char buf[64];
	int len = 0;
	while ((len = read(0, buf, 64)) > 0) {
		slen += len;
		if (!str)
			str = malloc(slen);
		else
			str = realloc(str, slen + len);
		memcpy(str + (slen - len - 1), buf, len);
		str[slen - 1] = 0;
	}
	int toks = 0;
	struct token **tok = tokenify(str, &toks);
	free(str);

	struct parser *p = parser_init(tok, toks);
	//struct stmt *s = parse_block(p);
	struct stmt *s = parse_function(p);
	if (s) {
		printf("AST Tree View\n");
		printf("------------------------------------\n");
		node_debug(s);
		printf("\n");
		printf("------------------------------------\n");

		//node_check(s, NULL);
		struct ir_stmt *stmt = generate(s);

		optimize(stmt);
		printf("TAC Intermediate Representation\n");
		printf("------------------------------------\n");
		ir_debug(stmt);
		printf("------------------------------------\n");

	       	struct ir_stmt *next=NULL;
		while (stmt) {
			next = stmt->next;
			ir_stmt_free(stmt);
			stmt = next;
		}
		node_free(s);
	}
	parser_free(p);
}
