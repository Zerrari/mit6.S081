#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int 
append(char argv[MAXARG][MAXARG])
{
	char temp[MAXARG];
	int pos = 0;
	char cur;
	int p = 0;
	while (read(0, &cur, 1) == 1)
	{
		if (cur == '\n')
		{
			temp[p] = '\0';
			strcpy(argv[pos], temp);
			p = 0;
			++pos;
		}
		else
		{
			temp[p++] = cur;
		}
	}
	if (p != 0)
	{
		temp[p] = '\0';
		strcpy(argv[pos++], temp);
	}
	return pos;
}

int
main(int argc, char* argv[])
{
	if (argc == 1)
	{
		fprintf(2, "usage: xargs command\n");
		exit(1);
	}

	int n = 1;
	int cur = 1;

	if (argc >= 3)
	{
		if (strcmp(argv[1], "-n") == 0)
		{
			n = atoi(argv[2]);
			cur = 3;
		}

	}

	char append_argv[MAXARG][MAXARG];
	int sum = append(append_argv);

	int iter = sum/n;
	int pos = 0;

	for (int i = 0; i < iter; ++i)
	{
		int pid = fork();
		if (pid == 0)
		{
			char* exec_argv[MAXARG];
			int i = cur;
			for (; i < argc; ++i)
			{
				exec_argv[i-cur] = argv[i];
			}
			i = 0;
			for (; i < n && pos < sum; ++i, ++pos)
			{
				exec_argv[argc-cur+i] = append_argv[pos];
			}
			exec_argv[argc-cur+i] = 0;
			for (int j = 0; j < argc-cur+i; ++j)
			{
			}
			exec(argv[cur], exec_argv);
			printf("exec error\n");
			exit(1);
		}
		else if (pid > 0)
		{
			wait((void*)0);
		}
		else
		{
			fprintf(2, "fork error\n");
			exit(1);
		}
	}

	exit(0);
}
