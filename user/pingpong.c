#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char* argv[])
{
	if (argc > 1)
	{
		fprintf(2, "usage: pingpong\n");
		exit(1);	
	}
	
	int pipe0[2];
	int pipe1[2];

	if (pipe(pipe0) || pipe(pipe1))
	{
		fprintf(2, "pipe error\n");
		exit(1);	
	}

	int pid = fork();

	if (pid == 0)
	{
		char a;
		close(pipe0[1]);
		close(pipe1[0]);
		read(pipe0[0], &a, 1);
		close(pipe0[0]);
		int pid1 = getpid();
		printf("%d: received ping\n", pid1);
		write(pipe1[1], &a, 1);
		close(pipe1[1]);
		exit(0);
	}
	else if (pid > 0)
	{
		char b;
		int pid2 = getpid();
		close(pipe0[0]);
		close(pipe1[1]);
		write(pipe0[1], "a", 1);
		close(pipe0[1]);
		read(pipe1[0], &b, 1);
		close(pipe1[0]);
		printf("%d: received pong\n", pid2);
		wait((int*)0);
	}
	else
	{
		fprintf(2, "fork error\n");
		exit(1);
	}
		
	exit(0);
}
