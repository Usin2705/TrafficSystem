#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <string.h> 
#include <signal.h>
#include <ctype.h> 
#include "utils.h"


//-lmosquitto in compile
#include <mosquitto.h> 

#define INPUT_SIZE 80

// Number of children
#define N 3

// Global variable that indicates how many children have terminated
int n = 0;

static volatile int keepRunning = 1;

void writeLog(char * logText) {
	FILE *fp;
	fp = fopen("log.txt", "a");	
	
	//https://stackoverflow.com/questions/9596945/how-to-get-appropriate-timestamp-in-c-for-logs
	time_t ltime; //calendar time	
	ltime = time(NULL);  //get current cal time
	char *timeText = asctime((localtime(&ltime))); //convert to string
	
	//https://stackoverflow.com/questions/9628637/how-can-i-get-rid-of-n-from-string-in-c 
	timeText[strlen(timeText) - 1] = 0; //Using quick trick to remove the newline in asctime
	
	//Using fprintf to ensure no NULL character written to the file.	
	ssize_t er = fprintf(fp, "%s: %s\n", timeText, logText);
	
	fclose(fp);
}

// Write signal handler for signal SIGCHLD here. 
void sigchld_handler(int sig_no) {	
	pid_t pid;
	int status;
	//pid = waitpid(-1, &status, 0); 	
	//if ( WIFEXITED(status) ) 
    while ((pid=waitpid(-1, &status, WNOHANG)) > 0) { 
		char logText[40] = {'0'};
        int exit_status = WEXITSTATUS(status);         
        printf("%d Child exited, status: %d\n", pid, exit_status);
		sprintf(logText, "%d Child exited, status: %d", pid, exit_status);		
		writeLog(logText);
		n++;
    } 	
}

// Write signal handler for signal SIGINT here. 
void sigint_handler(int sig_no) {	
	char userInput[INPUT_SIZE] = {'0'};
	printf("Press Y to stop the traffic system: ");
	fgets(userInput, INPUT_SIZE, stdin);
	if ((tolower(userInput[0]) == 'y') || (userInput[1] != '\n')) {
		printf("Start terminating child....\n");
		//signal(SIGTERM, SIG_IGN);
		kill(-getpid(), SIGTERM);
	} else {
		printf("Action aborted\n");
	}		
}

// Write signal handler for signal SIGTERM here. 
void sigterm_handler(int sig_no) {	
}


int main (void) {	
	int pipes[CHILD_MAX][2];
	pid_t pid;	
	int status = 0;
	int i = 0; 
	
	writeLog("Program start");		
	
	char userInput[INPUT_SIZE] = {'0'};
	printf("Press S to start the traffic system: ");
	fgets(userInput, INPUT_SIZE, stdin);
	while ((tolower(userInput[0]) != 's') || (userInput[1] != '\n')){
		printf("Press S to start the traffic system: ");	
		fgets(userInput, INPUT_SIZE, stdin);	
	}
	
	writeLog("Traffic system start");
	
	// Must be called before any other mosquitto functions.
	// This function is not thread safe.
	//NOT THREAD SAFE
	mosquitto_lib_init();
	
	// Signal for catching child exit
	signal(SIGCHLD, sigchld_handler);
	
	// Signal for terminate child (don't use SIGKILL)
	// https://turnoff.us/geek/dont-sigkill/
	signal(SIGTERM, sigterm_handler);
	
	// Straight up copy from Lab07b, block parent until all child exit
	sigset_t new_mask;
	sigset_t old_mask;
	
	sigemptyset(&new_mask);
	sigaddset(&new_mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &new_mask, &old_mask);
	
	for (i = 0; i < CHILD_MAX; i++ ){		
		pipe(pipes[i]);
	}
	
	signal(SIGINT, sigint_handler);	
	
	for (i = 0; i < CHILD_MAX; i++ ){
		sleep(1);
		if ((pid = fork()) < 0) {
			perror("Fork: error");
			exit(1);			
		
		} else if (pid == 0) {
			printf("Child %d started. Its pid is %d\n", i, getpid());
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
			
		} else {
			
		}			
	}
			
		
	
	while (n < N) {
		sigsuspend(&old_mask);
	}
		
	printf("All child terminated. Program exit\n");
	writeLog("All child terminated. Program exit");
	return (0);	
}