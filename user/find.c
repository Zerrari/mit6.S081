#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* 
fmtname(char* path)
{
 	char *p;

 	// Find first character after last slash.
 	for(p=path+strlen(path); p >= path && *p != '/'; p--)
   		;
  		p++;

  	// Return blank-padded name.
	return p;	
}

void 
find(char* dirname, char* filename)
{
 	int fd;
	struct dirent de;
	struct stat st;
	
 	if((fd = open(dirname, 0)) < 0)
	{
		fprintf(2, "ls: cannot open %s\n", dirname);
		return;
 	}

 	if(fstat(fd, &st) < 0)
	{
		fprintf(2, "ls: cannot stat %s\n", dirname);
		close(fd);
		return;
 	}

  	char buf[512], *p;

 	switch(st.type)
	{
  		case T_FILE:
			if (strcmp(filename, fmtname(dirname)) == 0)	
				printf("%s\n", dirname);
			break;

  		case T_DIR:
    		strcpy(buf, dirname);
    		p = buf+strlen(buf);
    		*p++ = '/';
    		while(read(fd, &de, sizeof(de)) == sizeof(de))
			{
      			if (de.inum == 0)
				{
        			continue;
				}
				if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
				{
					continue;
				}
				memmove(p, de.name, DIRSIZ);
			  	p[DIRSIZ] = 0;
			  	if(stat(buf, &st) < 0)
				{
        			//printf("ls: cannot stat %s\n", buf);
        			continue;
      			}
				find(buf, filename);
    		}
    		break;
  	}

 	close(fd);

	return;
}

int
main(int argc, char* argv[])
{
	if (argc != 3)
	{
		fprintf(2, "usage: find directory file\n");
		exit(1);
	}
	find(argv[1], argv[2]);
	exit(0);
}
