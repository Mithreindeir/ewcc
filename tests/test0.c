int foo() {
	int a = 2;
	int b = 8;
	int c = 4;
	int d;
	for (int j = 0; j <= 10; j++) {
		a = a * (j * (b / c));
		d = a * (j * (b / c));
	}
	if (a > b) {
		b = a - b;
	} else if (a < d) {
		c = d - a + b / 2;
	}
}

