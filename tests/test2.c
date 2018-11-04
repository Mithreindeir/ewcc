int strlen(char *s) {
	int len = 0;	
	while (s[len++] != 0);
	return len;
}
int main(int argc, char **argv) {

	int len[10];   
	for (int i = 0; i < 10 && i < argc; i++) {
		len[i] = strlen(argv[i]);
	}
	return 0;
}
