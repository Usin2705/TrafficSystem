#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>

//-lmosquitto in compile
#include <mosquitto.h> 

#include "../utils.h"


#define MQTT_HOST "ec2-13-48-23-39.eu-north-1.compute.amazonaws.com"
#define MQTT_PORT 1883
#define TOPIC_NAME "TRAFFIC"

//https://www.quora.com/What-is-the-difference-between-the-SIGINT-and-SIGTERM-signals-in-Linux-What%E2%80%99s-the-difference-between-the-SIGKILL-and-SIGSTOP-signals

//https://stackoverflow.com/questions/32479261/c-program-using-mosquitto-client-library-not-working
void on_publish(struct mosquitto *mosq, void *userdata, int mid)
{
    //mosquitto_disconnect(mosq);
	//printf("success publish\n");
}

#define RED_TIME 15
#define YELLOW_TIME 3
#define GREEN_TIME 15
#define DELAY_TIME 5 // Should never be higher than RED_TIME or GREEN_TIME
#define RED_STATE  0
#define GREEN_STATE 1

static volatile int keepRunning = 1;

// Write signal handler for signal SIGTERM here. 
void sigterm_handler(int sig_no) {	
	keepRunning = 0;
}


// Write signal handler for signal SIGINT here.
// Child process need to ignore SIGINT from parent
// https://stackoverflow.com/questions/6803395/child-process-receives-parents-sigint 
void sigint_handler(int sig_no) {		
}

void writeChildLog(char * logText, int lightNo) {
	FILE *fp;
	fp = fopen("lightLog.txt", "a");	
	
	//https://stackoverflow.com/questions/9596945/how-to-get-appropriate-timestamp-in-c-for-logs
	time_t ltime; //calendar time	
	ltime = time(NULL);  //get current cal time
	char *timeText = asctime((localtime(&ltime))); //convert to string
	
	//https://stackoverflow.com/questions/9628637/how-can-i-get-rid-of-n-from-string-in-c 
	timeText[strlen(timeText) - 1] = 0; //Using quick trick to remove the newline in asctime
	
	//Using fprintf to ensure no NULL character written to the file.
	//fprintf is thread safe POSIX
	ssize_t er = fprintf(fp, "%s: No %d, %s\n", timeText, lightNo, logText);
	
	fclose(fp);
}

void mosq_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
  
  switch(level){
    //case MOSQ_LOG_DEBUG:
    //case MOSQ_LOG_INFO:
    //case MOSQ_LOG_NOTICE:
    case MOSQ_LOG_WARNING:
    case MOSQ_LOG_ERR: {
      printf("\t\t\tCHILD: MQTT LOG %i:%s\n", level, str);
    }
  }
}

/*
	IF QoS = 1 (AT LEAST ONE), MQTT STILL SEND BUT SERVER CAN'T RECEIVE (WITH NO ERROR) AFTER ABOUT 1 MIN, DON'T KNOW WHY.
	QoS = 2 (EXACTLY ONCE), STILL FAIL, USE mosquitto_loop_start(mosq); BUT THE TRAFFIC1 OFF SIGNAL WON'T WORK
	SOMEHOW CHANGE QoS = 0 SOLVE THE PROBLEM. DON't KNOW WHY
*/

int main(int argc, char *argv[])	{
	printf("\t\t\tCHILD: Subprocess called\n");
	int i = atoi(argv[1]);
	int pipeR = atoi(argv[2]);
	int pipeW = atoi(argv[3]);
	char signalChr = 'A';
	int state = RED_STATE;
	
	signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigint_handler);	

	int keepAlive = 20;
	
	//MQTT connection must be kept alive longer than the cooldown of the traffic light, otherwise it will disconnected
	if (RED_TIME >= GREEN_TIME) {
		keepAlive = RED_TIME + 10;
	} else {
		keepAlive = GREEN_TIME + 10;
	}

	struct mosquitto *mosq;	
	
	int err = 0;
	
	//Create a new mosquitto client instance.
	mosq = mosquitto_new(argv[1], TRUE, NULL);	
	
	if (!mosq) {
		perror("Failed to create mosquitto instance");
		return(1);
	}
	
	mosquitto_log_callback_set(mosq, mosq_log_callback);
	
	//Set the publish callback.  This is called when a message initiated with mosquitto_publish has been sent to the broker successfully.
	//mosquitto_publish_callback_set(mosq, on_publish);
	
	//Connect to an MQTT broker.
	err = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, keepAlive);
	//mosq instance, timeout milliseconds to wait for network activity Set to 0 for instant return.  Set negative to use the default of 1000ms. Maxpacket = 1 (nolonger support)
	//mosquitto_loop_forever(mosq, -1, 1);
	
	if (err!=MOSQ_ERR_SUCCESS) {
		printf("CHILD: Error connect to MQTT broker: %s\n", mosquitto_strerror(err));
	}
	
	char buffer[9] = {'\0'};
	sprintf(buffer, "%s%d", TOPIC_NAME, i);
	
	//mosquitto_loop_start(mosq);
	
	while (keepRunning) {		
		if (i==0) {			
			//This is the first child
			//printf("child: %d, Publish RED color: %d\n", i, i);	
			
			//Publish a message on a given topic.			
			//RED--------------------------------------------------------------------------------------
			//mosq instance, NULL, topic, length of mess, message, QoS = 2 exactly once, retain = FALSE
			//printf("TOPIC CHILD %d: %s\n", i,buffer);
			err = mosquitto_publish(mosq, NULL, buffer, 3, "RED", 0, FALSE);			
			writeChildLog("Red light turn on", i);	
			//printf("\t\t\tCHILD: MQTT status RED %s: %s\n", buffer, mosquitto_strerror(err));			
			sleep(RED_TIME);			
			
			//YELLOW-----------------------------------------------------------------------------------
			//mosq instance, NULL, topic, length of mess, message, QoS = 2 exactly once, retain = FALSE
			err = mosquitto_publish(mosq, NULL, buffer, 6, "YELLOW", 0, FALSE);		
			//printf("MQTT status YELLOW %s: %s\n", buffer, mosquitto_strerror(err));						
			sleep(YELLOW_TIME);
			
			//Signal the next traffic light about status change
			write(pipeW, &signalChr, sizeof(signalChr));						
			
			//GREEN------------------------------------------------------------------------------------
			//mosq instance, NULL, topic, length of mess, message, QoS = 2 exactly once, retain = FALSE
			err = mosquitto_publish(mosq, NULL, buffer, 5, "GREEN", 0, FALSE);		
			//printf("MQTT status GREEN %s: %s\n", buffer, mosquitto_strerror(err));

			sleep(GREEN_TIME);
			
			//YELLOW-----------------------------------------------------------------------------------
			//mosq instance, NULL, topic, length of mess, message, QoS = 2 exactly once, retain = FALSE
			err = mosquitto_publish(mosq, NULL, buffer, 6, "YELLOW", 0, FALSE);		
			//printf("MQTT status YELLOW %s: %s\n", buffer, mosquitto_strerror(err));
			
			sleep(YELLOW_TIME);
			
			//Signal the next traffic light about status change
			write(pipeW, &signalChr, sizeof(signalChr));
		} else {
			//other childs.
			int readResult;
			while((readResult = read(pipeR, &signalChr, sizeof(signalChr))) < 0){
			}
			// Delay for syncing purpose
			sleep(DELAY_TIME);
			//printf("Child %d: run\n", i);				

			//YELLOW-----------------------------------------------------------------------------------
			//mosq instance, NULL, topic, length of mess, message, QoS = 2 exactly once, retain = FALSE
			//printf("TOPIC CHILD %d: %s\n", i,buffer);
			err = mosquitto_publish(mosq, NULL, buffer, 6, "YELLOW", 0, FALSE);		
			//printf("MQTT status YELLOW %s: %s\n", buffer, mosquitto_strerror(err));
			sleep(YELLOW_TIME);		
			
			//After yellow, signal the next traffic light
			if (i == CHILD_MAX-1) {
				//printf("This is last child.\n");
			} else {
				//Signal the next traffic light about status change
				write(pipeW, &signalChr, sizeof(signalChr));					
			}
			
			//GREEN------------------------------------------------------------------------------------
			if(state==RED_STATE) {
				//mosq instance, NULL, topic, length of mess, message, QoS = 2 exactly once, retain = FALSE
				err = mosquitto_publish(mosq, NULL, buffer, 5, "GREEN", 0, FALSE);		
				//printf("MQTT status GREEN %s: %s\n", buffer, mosquitto_strerror(err));
				state = GREEN_STATE;
				sleep(GREEN_TIME-DELAY_TIME);
				
			//RED------------------------------------------------------------------------------------
			} else {
				//mosq instance, NULL, topic, length of mess, message, QoS = 2 exactly once, retain = FALSE
				err = mosquitto_publish(mosq, NULL, buffer, 3, "RED", 0, FALSE);		
				writeChildLog("Red light turn on", i);
				//printf("MQTT status RED %s: %s\n", buffer, mosquitto_strerror(err));
				state = RED_STATE;
				sleep(RED_TIME-DELAY_TIME);
			}
		}
	}
	
	//printf("CHILD: %d terminated\n", i);
	close(pipeR); //close read			
	close(pipeW);
	
	//Send off signal to turn off the light
	err = mosquitto_publish(mosq, NULL, buffer, 3, "OFF", 0, FALSE);		
	writeChildLog("Light turn off", i);	
	
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	return 0;
}