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
		printf("Interference Graph\n");
		printf("------------------------------------\n");
		struct bb **bbs = cfg(stmt, &nbbs);
		if (bbs && bbs[0]) {
			/*
			for (int i = 0; i < nbbs; i++)
				printf("%d=(%d-%d)\n", i,bbs[i]->ln, bbs[i]->ln + bbs[i]->len);
			printf("digraph F {\n");
			for (int i = 0; i < nbbs; i++) {
				struct bb *n = bbs[i];
				for (int j = 0; j < n->nsucc; j++) {
					printf("\t\"(%d-%d)\" -> \"(%d-%d)\"\n", n->ln, n->ln+n->len, n->succ[j]->ln, n->succ[j]->ln + n->succ[j]->len);
				}
			}
			printf("}\n");
			*/
			struct bb **doms = calc_dom(bbs, nbbs);
			/*
			for (int i = 0; i < nbbs; i++) {
				printf("(%d-%d) - ", bbs[i]->ln, bbs[i]->ln + bbs[i]->len);
				if (doms[i])
					printf("doms[%d]= (%d-%d)\n", i,doms[i]->ln, doms[i]->ln+doms[i]->len);
				else printf("doms[%d] = null\n", i);
			}
			*/
			calc_df(bbs, doms, nbbs);
			set_ssa(bbs, nbbs);
			for (int i = 0; i < nbbs; i++) {
				bb_debug(bbs[i]);
			}
			printf("------------------------------------\n");
			proc_alloc(bbs, nbbs);
			for (int i = 0; i < nbbs; i++) {
				bb_debug(bbs[i]);
			}
			free(doms);
		}
		//optimize(stmt);
		printf("------------------------------------\n");
		printf("After Register Rewriting:\n");
		ir_debug(stmt);
		for (int i = 0; bbs && i < nbbs; i++) {
			bb_free(bbs[i]);
		}
		free(bbs);
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
