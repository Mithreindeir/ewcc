int main()  
{
	int x = 0;
	for (int i = 0; i < 10; i++) {
		x = x + (x+i) * i * (i-x);
	}
	return 0;
}
