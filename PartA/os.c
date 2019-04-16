#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

# define BUFFSIZE 100

char command[BUFFSIZE];
char *command_tok[BUFFSIZE];
int command_tokSize;
int pointer;

pid_t pid,status;

int ProcessType()
{
	int i;
	for ( i = 0; i < command_tokSize; ++i)
	{
		if(strcmp(command_tok[i],"&")==0)				//BackGround Check
		{
			return 1;       
		}
		else if(strcmp(command_tok[i],"<")==0)			//STDIN check	
		{
			return 2;       
		}
		else if(strcmp(command_tok[i],">")==0)			//STDOUT check
		{
			command_tok[i] = NULL;
			pointer = i;
			return 3;       
		}
	}
	if( i == command_tokSize)							//ForGround
	{
		return 4;
	}
}

label()
{
	sleep(0.5);
	printf("minish->");
}

input()
{
	fgets(command, BUFFSIZE, stdin);
	command[strcspn(command, "\n")] = 0; 			// Make end of string by 0

	char *p = strtok(command, " ");

	command_tokSize = 0;

	while (p) 
	{
		command_tok[command_tokSize++] = p;
		p = strtok(NULL, " ");
	}

	command_tok[command_tokSize] = NULL;  
}

void handler()
{

	int r = waitpid(pid,&status,WNOHANG);                        //Wait for particular child process & return immediately
}
void fun_1(int pid)
{
	int namelen = strcspn(command, "&");
	command_tok[namelen - 2] = NULL;                            // Truncate "&"

	if(pid == 0)
	{

		int r =  execvp(command_tok[0], command_tok);
		if(r<0)
		{
			printf("Exec Faield: Invalid command\n");

		}
	}

	if(pid < 0)
	{
		printf("Fork Failed\n");
	}
	if(pid > 0)
	{
		printf("Process: %d running in background\n",pid );
	}
}

void fun_2(int pid)
{
	command_tok[1] = command_tok[2];						// Truncate "<"
	command_tok[2] = NULL;
	int fd;
	if(pid == 0)
	{
		fd = open(command_tok[1],O_RDONLY);
		int d = dup2(fd,0);
		if(d==0)
		{
			close(fd);
			int r = execvp(command_tok[0], command_tok);  
			if(r<0)
			{
				printf("Exec Failed: Invalid command\n");
			}   
		}
		else
		{
			perror("Dup Error:");
			close(fd);
			exit(0);
		}
	}

	if(pid > 0)
	{
		//Do nothing
	}
	if(pid < 0)
	{
		printf("Fork Failed\n");
	}
}
fun_3(int pid)
{
	int fd;
	if(pid == 0)
	{
		fd = creat(command_tok[pointer+1],0644);
		int d = dup2(fd,1);
		if(d<0)
		{
			perror("Dup Error:");
			close(fd);
			exit(0);

		}
		else
		{
			close(fd);
			int r =  execvp(command_tok[0], command_tok);    
			if(r<0)
			{
				printf("Exec Failed: Invalid command\n");
			}  
		}
	}

	if(pid > 0)
	{
		// Do nothing
	}
	if(pid < 0)
	{
		printf("Fork Failed\n");
	}
}

void fun_4()
{
	if(pid == 0)
	{
		int r = execvp(command_tok[0], command_tok);
		if(r<0)
		{
			printf("Exec Failed: Invalid command\n");
		}
	}
	if(pid > 0)
	{
		int r = wait(NULL);
		if(r==-1)
		{
			printf("Wait Failed\n");	
		}	
	}
	if(pid < 0)
	{
		printf("Fork Failed\n");
	}
}

int main(int argc, char *argv[])
{
	while(1)
	{
		label();														// minish label
		input();														// generate token of input

		int type = ProcessType();										// Return Process Type
		signal(SIGCHLD, handler);                                       //Signal Handler to avoid Zombie Process

		pid = fork();
		switch(type)
		{
			case 1:                                                      // Background Process
			{
				fun_1(pid);
				break;
			}
			case 2:																// STDIN process
			{
				fun_2(pid);
				break;
			}
			case 3:																// STDOUT process
			{
				fun_3(pid);
				break;
			}
			case 4:                                                          		//Foreground Process
			{
				fun_4(pid);
				break;
			}
		}
	}   
	return 0;
}