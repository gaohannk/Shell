/*
Created by Han Gao. This is for Operate System Course Homwork 4
Modify Feb 16
*/
#define MAX_SUB_COMMANDS 5
#define MAX_ARGS 10
#define MAX_CHILD 5
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

struct SubCommand
{
	char *line;
	char *argv[MAX_ARGS];
};
struct Command
{
	struct SubCommand sub_commands[MAX_SUB_COMMANDS];
	int num_sub_commands;
	char *stdin_redirect;
	char *stdout_redirect;
	int background;
};

void ReadCommand(char *line, struct Command * command);
void PrintCommand(struct Command * command);
void ReadArgs(char *in, char **argv, int size);
void PrintArgs(char **argv);
void ReadRedirectsAndBackground(struct Command *command);
void prompt();
void do_cd();
void handle_build_in_command(struct Command *command);
void handler(int sig);
void interupt_handler(int sig);
int isbuildin(struct Command *command);

pid_t pid;
int childpid[MAX_CHILD];
int flag[MAX_CHILD];

int main(){
	char line[200];
	int sig[10];
	printf("-----------------Super shell---------------------------\n");
	printf("Hello %s ! Welcome to your own shell.\n", getenv("USER"));
	printf("-------------------------------------------------------\n");
	signal(SIGINT, interupt_handler);
	while(1){
	// Read a string from the user
	int id=0;
	while(id<MAX_CHILD){
		if(flag[id]==1){
			printf("[[%d]]\n", childpid[id]);
			childpid[id]=0;
			flag[id]=0;
		}
		id++;
	}
	prompt();
	fgets(line, sizeof(line), stdin);
	// Extract arguments and print them
	struct Command *command= malloc(sizeof(struct Command));
	//printf("%lu\n",sizeof(struct Command));
	ReadCommand(line, command);
	ReadRedirectsAndBackground(command);
	    int i=0;
		while(i<command->num_sub_commands){
			//printf("Sub Command %d\n",i);
		    if(isbuildin(command)){
		   		handle_build_in_command(command);
		    	break;
			}
			int fds[2];
			int lastfds[2];
			if(i!=0){
				lastfds[0]=fds[0];
				lastfds[1]=fds[1];
			}
			if (pipe(fds) == -1){
				perror("pipe");
				return 1;
			}
			int ret = fork();
			if (ret < 0){
				fprintf(stderr, "fork failed\n");
				exit(1);
			} else if (ret == 0){
				if(i!=command->num_sub_commands-1){
					close(fds[0]);
					close(1);
					dup(fds[1]);
				}
				if(i!=0){
					close(lastfds[1]);
					close(0);
					dup(lastfds[0]);
				}
				if(i==command->num_sub_commands-1&&command->background==1){
					printf("[%d]\n", getpid());
					pid=getpid();
					//printf("%d\n", pid);
					//sprintf(pid,"%d",getpid());
					write(fds[1],"block",10);
				}
				if(i==0&&command->stdin_redirect!=NULL){
 					fds[0] = open(command->stdin_redirect, O_RDONLY, 0644);		
 					if(fds[0]<0){
						printf("%s: File not found\n", command->stdin_redirect);
						exit(0);		
 					}
					close(0);
					dup(fds[0]);
				}
				if(i==command->num_sub_commands-1&&command->stdout_redirect!=NULL){
					fds[1] = open(command->stdout_redirect, O_CREAT | O_WRONLY, 0644);
					if(fds[1]<0){
						printf("%s: Cannot create file\n", command->stdout_redirect);	
						exit(0);
					}
					close(1);
					dup(fds[1]);
				}
				//PrintCommand(command);
				int err=execvp(command->sub_commands[i].argv[0],command->sub_commands[i].argv);
				if(err){
					printf("%s: Command not found\n", command->sub_commands[i].argv[0]);
					exit(0);
				}
			} else if (ret > 0){
				//if(i==0)
				//close(lastfds[0]);
				//close(lastfds[1]);
				//close(fds[0]);
				close(fds[1]);
				if(command->background==0)
					wait(NULL);
				else{
					if(i==command->num_sub_commands-1){
						int c_id=0;
						while(c_id<MAX_CHILD){
							if(childpid[c_id]==0){
								childpid[c_id]=ret;
								break;
							}
							c_id++;
						}
						signal(SIGCHLD,handler);
						char *buf;
						read(fds[0],buf,10);
						close(fds[0]);
					}
				}
				i++;
			}
		}
	
	}
}

void ReadRedirectsAndBackground(struct Command *command){
	int i;
	if(command->num_sub_commands==0)
		return;
	//Only consider the last sub command 
	for(i=MAX_ARGS-1;i>=0;i--){
		char *argv=command->sub_commands[command->num_sub_commands-1].argv[i];
		if(argv==NULL)
			continue;
		else if(!strcmp(argv,"&")){
			command->background=1;
			command->sub_commands[command->num_sub_commands-1].argv[i]=NULL;
			argv=NULL;
		}else if(!strcmp(argv,">")){
			command->stdout_redirect=command->sub_commands[command->num_sub_commands-1].argv[i+1];
			command->sub_commands[command->num_sub_commands-1].argv[i+1]=NULL;
			command->sub_commands[command->num_sub_commands-1].argv[i]=NULL;
		}else if(!strcmp(argv,"<")){
			command->stdin_redirect=command->sub_commands[command->num_sub_commands-1].argv[i+1];
			command->sub_commands[command->num_sub_commands-1].argv[i+1]=NULL;
			command->sub_commands[command->num_sub_commands-1].argv[i]=NULL;
		}
	}
}

void ReadCommand(char *line, struct Command *command){
	line[strlen(line)-1]='\0';
	char *pch = strtok(line, "|");
	int id=0;
	command->num_sub_commands=0;
	while(pch!=NULL){
		command->sub_commands[id].line=malloc(sizeof(pch));
		command->sub_commands[id].line=strdup(pch);
		command->num_sub_commands++;
		pch=strtok(NULL,"|");
		id++;
	}
	id=0;
	while(id<command->num_sub_commands){
		ReadArgs(command->sub_commands[id].line,command->sub_commands[id].argv,MAX_ARGS);
		id++;
	} 
}
void PrintCommand(struct Command * command){
	int i=0;
	while(i<command->num_sub_commands){
		printf("Command %d:\n", i);
		PrintArgs(command->sub_commands[i].argv);
		printf("\n");
		i++;
	}
	// print stdin stdout and &
	if(command->stdin_redirect!=NULL)
		printf("Redirect stdin: %s\n", command->stdin_redirect);
	if(command->stdout_redirect!=NULL)
		printf("Redirect stdout: %s\n", command->stdout_redirect);
	if(command->background==1)
		printf("Background: yes\n");
	else
		printf("Background: no\n");
}

void ReadArgs(char *in, char **argv, int size){
	char *dup =strdup(in);
	dup[strlen(dup)]='\0';
	char *pch = strtok(dup, " ");
	while(pch!=NULL){
		*argv=pch;
		argv++;	
		pch=strtok(NULL," ");
	}
	//Don't forget Set NULL to argv
	*argv=NULL;
}

void PrintArgs(char ** argv){
	int i=0;
	while(*argv){
		printf("argv[%d] = '%s'\n",i,*argv);
		argv++;
		i++;
	}
}

void prompt(){
	char buf[200];
    getcwd(buf, sizeof(buf));
	printf("%s $ ", buf);
}

void do_cd(char *path){
	if(path==NULL){
		chdir(getenv("HOME"));
		return;
	}
	if(path[0]=='~'){
		char *home=getenv("HOME");
		path++;
		strcat(home,path);
		//printf("%s\n",home);
		path=home;
	}
	int err=chdir(path);
	if(err==-1)
		printf("%s No such file or directory\n", path);
}

int isbuildin(struct Command *command){
	char *buildincommand[7]={"cd","exit","quit","echo","print","exit",NULL};
	int i=0;
	while(buildincommand[i]!=NULL){
		if(!strcmp(buildincommand[i],command->sub_commands[0].argv[0]))
			return 1;
		i++;
	}
	return 0;
}

void handle_build_in_command(struct Command *command){
			if(!strcmp(command->sub_commands[0].argv[0],"cd")){
				do_cd(command->sub_commands[0].argv[1]);
			}
			if(!strcmp(command->sub_commands[0].argv[0],"exit")){
				printf("ByeBye\n");
				exit(0);
			}
			if(!strcmp(command->sub_commands[0].argv[0],"quit")){
				printf("ByeBye\n");
				exit(0);
			}
			if(!strcmp(command->sub_commands[0].argv[0],"echo")){
				printf("%s\n", command->sub_commands[0].argv[1]);
			}
			if(!strcmp(command->sub_commands[0].argv[0],"printf")){
				printf("%s", command->sub_commands[0].argv[1]);
			}
			if(!strcmp(command->sub_commands[0].argv[0],"alias")){
				//printf("%s", command->sub_commands[0].argv[1]);
			}
			if(!strcmp(command->sub_commands[0].argv[0],"umask")){
				//printf("%s", command->sub_commands[0].argv[1]);
			}
			if(!strcmp(command->sub_commands[0].argv[0],"pwd")){
				char buf[200];
    			getcwd(buf, sizeof(buf));
				printf("%s ", buf);
			}
			// if(!strcmp(command->sub_commands[0].argv[0],"eval")){
			// 	printf("%s", command->sub_commands[0].argv[1]);
			// }
			//if(!strcmp(command->sub_commands[0].argv[0],"eval")){
			//	printf("%s", command->sub_commands[0].argv[1]);
			//}

}

void handler(int sig){
	//printf("%d",sig);
	if(sig==SIGCHLD){
		int ret=waitpid(-1,NULL,WNOHANG);
		//printf("I am waiting %d\n", ret);
		int i=0;
		while(i<MAX_CHILD){
			if(childpid[i]==ret){
				//printf("[[%d]]\n", childpid[i]);
				flag[i]=1;
			}
			i++;
		}
	}
}
void interupt_handler(int sig){
	if(sig==SIGINT)
		kill(pid,SIGTERM);
}



