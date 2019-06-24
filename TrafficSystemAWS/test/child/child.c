#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include "../utils.h"

int main(int argc, char *argv[])	{	
	int i = atoi(argv[1]);
	int pipeR = atoi(argv[2]);
	int pipeW = atoi(argv[3]);
	char chrA = 'A';
	char chr = '0';
	
	if (i==0) {
		//This is the first child
		printf("child: %d, -------------write to: %d\n", i, i+1);
		write(pipeW, &chrA, sizeof(chrA));
		sleep(1);		
	} else {
		//other childs.
		int readResult;
		while((readResult = read(pipeR, &chr, sizeof(chr))) < 0){
		}
		printf("Success read in pipe %d: character now is %c\n", i, chr);
		close(pipeR); //close read
		sleep(1);		
		
		if (i == CHILD_MAX-1) {
			printf("This is last child. Final char %c\n", chr);
					
		} else {
			chr = chr + 1;
			printf("child: %d, -------------write to: %d\n", i, i+1);
			write(pipeW, &chr, sizeof(chr));
		}
	}
	
	close(pipeW);			
	exit(0);
}