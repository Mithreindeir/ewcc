int foo() {
	int sum = 0;
	for (int i = 0; i < 10; i++) {
		sum = sum + i;
		if (sum > 0) {
			sum = sum / 2;
		} else {
			sum = sum * 2;
		}
	}
}
