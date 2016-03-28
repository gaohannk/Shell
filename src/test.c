#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>


int main(){
	char *arg[1];
	arg[0]="env";
	execvp(arg[0],arg);
}