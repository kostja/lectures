#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sort.h"

#if 0
enum { SIZE = 4194304 };
#endif
enum { SIZE = 4096 };
int data[SIZE];

void
generate(int *data, int size)
{
	srand((unsigned) time(0));
	for (int i = 0; i < size; i++)
		data[i] = rand();
}

int
intcmp(const void *p_a, const void *p_b)
{
	return *(int *) p_a -  *(int *) p_b;
}

void
output(int *data, int size)
{
	for (int i = 0; i < size; i++)
		printf("%d\n", data[i]);
}

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	generate(data, SIZE);
	sort(data, SIZE, sizeof(*data), intcmp);
	output(data, SIZE);

	return 0;
}
