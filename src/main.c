#include <stdio.h>
#include <unistd.h>
#include "lexer.h"
#include "parser.h"
#include "checker.h"

int main() {
	char *str = NULL;
	int slen = 1;
	char buf[64];
	int len = 0;
	while ((len=read(0, buf, 64)) > 0) {
		slen += len;
		if (!str) str = malloc(slen);
		else str = realloc(str, slen+len);
		memcpy(str+(slen-len-1), buf, len);
		str[slen-1] = 0;
	}
	int toks = 0;
	struct token **tok=tokenify(str, &toks);
	free(str);

	struct parser *p = parser_init(tok, toks);
	struct stmt *s = parse_stmt(p);
	if (s) {
		printf("\n");
		node_print(s);
		printf("\n");
		node_check(s, NULL);
		node_free(s);
	}
	parser_free(p);
}
