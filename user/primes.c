#include "kernel/types.h"
#include "user/user.h"

void wrap(int value, char* tar)
{
	for (int i = 0; i < 4; ++i)
	{
		tar[i] = (value >> (24 - 8*i)) & 0xff;
	}
	return;
}

int unwrap(char* value)
{
	int cur = 0;
	for (int i = 0; i < 4; ++i)
	{
		cur <<= 8;
		cur |= value[i];
	}
	return cur;
}

void
primes(int* arr)
{
	int p[2];
	if (pipe(p) != 0)
	{
		fprintf(1, "p error\n");
		exit(1);
	}
	int pid = fork();		
	if (pid == 0)
	{
		int accept[36];
		int sum = 0;
		for (int i = 2; i <= 35; ++i)
			arr[i] = 0;
		close(p[1]);
		char a[4];
		while (read(p[0], a, 4) != 0)
		{
			int cur = unwrap(a);
			accept[cur] = 1;
			++sum;
		}
		close(p[0]);
		if (sum != 0)
			primes(accept);
		exit(0);
	}
	else if (pid > 0)
	{
		close(p[0]);
		int i = 2;
		int cur = 0;
		for (; i <= 35; ++i)
			if (arr[i] == 1)
			{
				printf("prime %d\n", i);
				cur = i;
				++i;
				break;
			}
		for (; i <= 35; ++i)
		{

			if (arr[i] == 1 && i % cur != 0)
			{
				char temp[4];
				wrap(i, temp);
				write(p[1], temp, 4);
			}
		}
		close(p[1]);
		wait((void*)0);
	}
	else
	{
		fprintf(1, "fork error\n");
		exit(1);
	}
	return;
}

int
main(int argc, char *argv[])
{
	int arr[36];
	for (int i = 2; i <= 35; ++i)
		arr[i] = 1;
	primes(arr);
	exit(0);
}
