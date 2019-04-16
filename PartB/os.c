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
int killarray[100];
int killarraycnt=0;
int killpipe[100];
int killpipecnt=0;
int Foreground;
int pidMain;

void handlePipe();															// Handle pipes in shell
void cntrlhandler();														// SIGINT controler 
int ProcessType();															// Figure out type of process, fg/bg/</>
void handler();																// SIGCHID handler
void input();																// Take user input and tokanize	
int inputMode();															// Check pipe or differnt mode
void label();																// Minish lable

/*
	In backgroundProcess run a process with & as suffix in background: 	
*/
void backgroundProcess()
{
	pid = fork();
	if (pid < 0)
	{
		printf("Fork Failed\n");
	}
	if(pid == 0)
	{
		int r =  execvp(command_tok[0], command_tok);
		if(r<0)
		{
			printf("Exec Faield: Invalid command\n");
		}
	}
	if(pid > 0)															// Not calling wait in parent, so parent does not wait for child's execution			
	{
		killarray[killarraycnt++] = pid; 								// Collecting child's PID for exit.
		printf("Process: %d running in background\n",pid );
	}
}
/*
	Redirect input from a file to process
*/
void stdinProcess()
{
	pid = fork();
	if (pid < 0)
	{
		printf("Fork Failed\n");
	}
	
	command_tok[1] = command_tok[2];															// Truncate "<"
	command_tok[2] = NULL;
	
	int fd;
	
	if(pid == 0)
	{
		fd = open(command_tok[1],O_RDONLY);														//Open file in readonly mode
		if(fd < 0)
		{
			printf("Open Failed\n");
		}
		int d = dup2(fd,0);																		//Duplicate fd of stdin 
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
}
/*
	Redirect output of the process to a file
*/
void stdoutProcess()
{
	pid = fork();
		if (pid < 0)
		{
			printf("Fork Failed\n");
		}
	int fd;
	if(pid == 0)
	{
		fd = creat(command_tok[pointer+1],0644);										// Create or open a file in write mode
		if (fd < 0)
		{
			printf("Fd Failed to create a file\n");
		}
		int d = dup2(fd,1);																//Duplicate fd of stdout
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
}
/*
	Default way to run a process.
*/
void foregroundProcess()
{
	
	pid = fork();
	if (pid < 0)
	{
		printf("Fork Failed\n");
	}
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
		Foreground = pid;
		int r = wait(NULL);																	// Waits until child is executed.
		if(r==-1)
		{
			printf("Wait Failed\n");	
		}	
	}
}


int main(int argc, char *argv[])
{
	if(signal(SIGINT, SIG_IGN)==SIG_ERR)
	{
		printf("Error: SIGNT Failed\n");
	}
	pidMain = getpid();
	while(1)
	{
		label();																				// minish label
		int r = inputMode();																	//Check pipe in input 
		
		if(r==1)
		{
			input();
			int type=0;
			type = ProcessType();																// Return Process Type                                					
			switch(type)
			{
				case 1:                                                    						// Background Process
				{
					if(signal(SIGINT, SIG_IGN)==SIG_ERR)
					{
						printf("Error: SIGNT Failed\n");
					}
					if(signal(SIGCHLD, handler)==SIG_ERR)
					{
						printf("Signal Error: Failed to catch SIGCHLD\n");
					}																			//Signal Handler to avoid Zombie Process
					backgroundProcess();
					break;
				}
				case 2:																			// STDIN process
				{
					stdinProcess();
					break;
				}
				case 3:																			// STDOUT process
				{
					stdoutProcess();
					break;
				}
				case 4:                                                          				//Foreground Process
				{	
					if(signal(SIGINT, cntrlhandler)==SIG_ERR)
					{				
						printf("Signal Error: Failed to catch SIGINT\n");
					}
					foregroundProcess();
					break;
				}
				case 5:
				{
					int i;
					for ( i = 0; i < killarraycnt; ++i)
					{
						kill(killarray[i],15);
					}
					return 0;
				}
			}
		}
		else
		{
			handlePipe();																		// Implement Pipe
		}   
	}	
		
	return 0;
}

void handlePipe()
{
	int fd[2];
	int fdd = 0;
	int in=0,out=0;
		

	char *p = strtok(command, "|");														// Tok with |
	command_tokSize = 0;
	int flag =0,flag1=0 ;

	while (p) 
	{
			command_tok[command_tokSize++] = p;
			p = strtok(NULL, "|");
	}

	command_tok[command_tokSize] = NULL;  
	int i=0;
	int tempCommandSize = command_tokSize;
	while(i<command_tokSize)
	{
		char *tempBuff[BUFFSIZE];
		int tempBuffCnt=0;

		char * q = strtok(command_tok[i], " ");										// Tok with " "
		tempBuffCnt = 0;
		while(q)
		{
			tempBuff[tempBuffCnt++] = q;
			q = strtok(NULL, " ");
		}
		tempBuff[tempBuffCnt] = NULL;
	
		int  r =pipe(fd);															// Create a pipe with fd
		if(r<0)
		{
			printf("Pipe Failed\n");
		}	
		int j;
		pid = fork();
		if(pid == 0)
		{
			int	s = dup2(fdd, 0);
			if (s<0)
			{	
				perror("Dup Error:");
			}	
			
			if(i !=(command_tokSize-1))
			{
				int t = dup2(fd[1], 1);
				if (t<0)
				{
					perror("Dup Error:");
				}
			}
						
			for(j=0;j<tempBuffCnt;j++)														// I/O direction check
			{	
				if(strcmp(tempBuff[j],"<")==0)
				{	
					in = open(tempBuff[j+1],O_RDONLY);	
					tempBuff[j] = tempBuff[j+1];
					tempBuff[j+1]=NULL;
					int	s =dup2(in,0);
					if (s<0)
					{
						perror("Dup Failed");
					}
					int u = close(in);
					if(u<0)
					{
						printf("Close Faield\n");
					}
					break;
				}
				if(strcmp(tempBuff[j],">")==0)
				{	
					out = creat(tempBuff[j+1],0644);
					tempBuff[j]=NULL;	
					int	s = dup2(out, 1);
					if (s<0)
					{
						perror("Dup Failed");
					}
					int u = close(out);
					if(u<0)
					{
						printf("Close Faield\n");
					}
					break;
				}
			}

			int u = close(fd[0]);
			if(u<0)
			{
				printf("Close Faield\n");
			}
			int r = execvp(tempBuff[0],tempBuff);
			if (r==-1)
			{
				printf("Exec Failed\n");
			}
		}
		else if(pid > 0)
		{
			killpipe[killpipecnt] = pid;
			int r = wait(NULL); 		 
			if(r!=-1)
			{
				close(fd[1]);
				fdd = fd[0];
				i++;
			}
			else
			{
				printf("Wait Failed\n");
			}
		}
	}
}

void cntrlhandler(){}

int ProcessType()
{
	int i;
	for ( i = 0; i < command_tokSize; ++i)
	{
		if(strcmp(command_tok[i],"&")==0)													//BackGround Check
		{
			command_tok[i] = NULL;
			command_tokSize--;
			return 1;       
		}
		else if(strcmp(command_tok[i],"<")==0)												//STDIN check	
		{
			return 2;       
		}
		else if(strcmp(command_tok[i],">")==0)												//STDOUT check
		{
			command_tok[i] = NULL;
			pointer = i;
			return 3;       
		}
		else if(strcmp(command_tok[i],"exit")==0)											//exit check
		{
			return 5;
		}
	}
	if( i == command_tokSize)																//ForeGround
	{
		return 4;
	}
}

void label()
{
	sleep(1);
	printf("minish->");
}
int inputMode()
{
	fgets(command, BUFFSIZE, stdin);
	
	command[strcspn(command, "\n")] = 0; 													// Make end of string = 0
	int c = strcspn(command, "0");
	int d = strcspn(command, "|");
	
	if(c==d)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

void input()
{
	int i;

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
	int r = waitpid(pid,&status,WNOHANG);                        							//Wait for particular child process & return immediately
}



