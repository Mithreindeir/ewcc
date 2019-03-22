#include <stdio.h>
#include <unistd.h>

#include "frontend/parser.h"
#include "frontend/checker.h"
#include "debug.h"
#include "tac.h"
#include "peep.h"
#include "ralloc.h"
#include "cfg.h"
#include "ssa.h"
#include "x86gen.h"
#include "memory.h"

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

		struct ir_stmt *stmt = generate(s, get_scope(p));

		optimize(stmt);
		printf("TAC Intermediate Representation\n");
		printf("------------------------------------\n");
		ir_debug(stmt);
		int nbbs = 0;
		printf("------------------------------------\n");
		struct bb **bbs = NULL;
		//struct bb **bbs = cfg(stmt, &nbbs);
		bbs = cfg(stmt, &nbbs);
		do {
			/*bb func separation is getting screwed up here*/
			if (bbs && bbs[0]) {
				struct bb **doms = calc_dom(bbs, nbbs);
				calc_df(bbs, doms, nbbs);
				set_ssa(bbs, nbbs);
				proc_alloc(bbs, nbbs);
				free(doms);
			}
			//optimize(stmt);
			proc_asn(bbs, nbbs);
			printf("------------------------------------\n");
			printf("After Register Rewriting:\n");
			for (int i = 0; i < nbbs; i++) {
					bb_debug(bbs[i]);
					printf("----------------\n");
			}
			//ir_debug(stmt);
			for (int i = 0; bbs && i < nbbs; i++) {
				bb_free(bbs[i]);
			}
			free(bbs);
			bbs = NULL;
			printf("------------------------------------\n");
		} while ((bbs = cfg(NULL, &nbbs)));
		ir_debug(stmt);
		optimize(stmt);
		printf("------------------------------------\n");
		instr(stmt);

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
