int fib(int n) {
	if (n <= 1) return n;
	else {
		return fib(n-1) + fib(n-2);
	}
}
int main() {
	for (int i = 0; i < 10; i++) {
		int v = fib(i);
	}
	return 0;
}
