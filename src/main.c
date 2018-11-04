#include <stdio.h>
#include <unistd.h>

#include "frontend/parser.h"
#include "frontend/checker.h"
#include "debug.h"
#include "tac.h"
#include "peep.h"
#include "ralloc.h"
#include "live.h"

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
	struct stmt *s = parse_unit(p);
	if (s) {
		node_check(&s, NULL);
		printf("AST Tree View\n");
		printf("------------------------------------\n");
		node_debug(s);
		printf("\n");
		printf("------------------------------------\n");

		struct ir_stmt *stmt = generate(s);

		optimize(stmt);
		printf("TAC Intermediate Representation\n");
		printf("------------------------------------\n");
		ir_debug(stmt);
		int nbbs = 0;
		printf("------------------------------------\n");
		printf("Interference Graph\n");
		printf("------------------------------------\n");
		struct bb **bbs = cfg(stmt, &nbbs);
		if (bbs && bbs[0])
			bb_creg(bbs[0]);
		optimize(stmt);
		printf("------------------------------------\n");
		printf("After Register Rewriting:\n");
		ir_debug(stmt);
		for (int i = 0; bbs && i < nbbs; i++) {
			bb_free(bbs[i]);
		}
		free(bbs);
		printf("------------------------------------\n");
		printf("Initial Instruction Selection:\n");

		struct ir_stmt *next = NULL;
		while (stmt) {
			next = stmt->next;
			ir_stmt_free(stmt);
			stmt = next;
		}
		node_free(s);
	}
	parser_free(p);
}
