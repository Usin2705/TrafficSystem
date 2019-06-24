#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

//-lmosquitto in compile
#include <mosquitto.h> 

int main (void) {	
	int pipes[CHILD_MAX][2];
	pid_t pid;	
	int status = 0;
	int i = 0; 
	
	mosquitto_lib_init();
		
	for (i = 0; i < CHILD_MAX; i++ ){		
		pipe(pipes[i]);
	}
	
	for (i = 0; i < CHILD_MAX; i++ ){
		sleep(1);
		if ((pid = fork()) < 0) {
			perror("Fork: error");
			exit(1);			
		
		} else if (pid == 0) {			
			int j;
			for (j = 0; j < CHILD_MAX; j++) {
				//close all other read
				if (j!=(i)) {
					close(pipes[j][0]);
				}
				
				//close all other write:
				if (j!=(i+1)) {
					close(pipes[j][1]);
				}
			}
			//child			
			char iChar[2];
			char pipeRChar[2];
			char pipeWChar[2];			
			snprintf(iChar, 3, "%d", i);
			snprintf(pipeRChar, 3, "%d", pipes[i][0]);
			snprintf(pipeWChar, 3, "%d", pipes[i+1][1]);
			execlp("./SubProcesses/subprocess.exe", "subprocess.exe", iChar, pipeRChar, pipeWChar, (char*) NULL);			
		}		
	}
			
	
	while (wait(&status) > 0);	
	printf("This is parent. All child terminated\n");
	return (0);	
}