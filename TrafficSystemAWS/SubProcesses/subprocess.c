#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> 


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


int main(int argc, char *argv[])	{
	printf("subprocess called\n");
	int i = atoi(argv[1]);
	int pipeR = atoi(argv[2]);
	int pipeW = atoi(argv[3]);
	char signalChr = 'A';

	int keepAlive = 10;
	
	struct mosquitto *mosq;	
	
	int err = 0;
	
	
	//Must be called before any other mosquitto functions.
	//This function is not thread safe.
	//NOT THREAD SAFE
	
	
	//Create a new mosquitto client instance.
	mosq = mosquitto_new(argv[1], TRUE, NULL);
	
	if (!mosq) {
		perror("Failed to create mosquitto instance");
		return(1);
	}
	
	//Set the publish callback.  This is called when a message initiated with mosquitto_publish has been sent to the broker successfully.
	//mosquitto_publish_callback_set(mosq, on_publish);
	
	//Connect to an MQTT broker.
	err = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, keepAlive);
	
	if (err!=MOSQ_ERR_SUCCESS) {
		printf("Error connect to MQTT broker: %s\n", mosquitto_strerror(err));
	}
	
	char buffer[9] = {'\0'};
	sprintf(buffer, "%s%d", TOPIC_NAME, i+1); //AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA ++++++++++++++++++++++++++++++++ 111111111111111111
	
	
	if (i==0) {
		//This is the first child
		printf("child: %d, Publish RED color: %d\n", i, i+1);
		
		//Publish a message on a given topic.
		//mosq intanace, NULL, topic, length of mess, message, 1, TRUE
		printf("TOPIC CHILD %d: %s\n", i+1,buffer);
		err = mosquitto_publish(mosq, NULL, buffer, 3, "RED", 1, TRUE);		
		printf("MQTT status RED CHILD 1: %s\n", mosquitto_strerror(err));			
		sleep(5);
		
		write(pipeW, &signalChr, sizeof(signalChr));
		
		sleep(5);		
		err = mosquitto_publish(mosq, NULL, buffer, 6, "YELLOW", 1, TRUE);		
		printf("MQTT status YELLOW CHILD 1: %s\n", mosquitto_strerror(err));			
		
		sleep(3);
		err = mosquitto_publish(mosq, NULL, buffer, 5, "GREEN", 1, TRUE);				
		
	} else {
		//other childs.
		int readResult;
		while((readResult = read(pipeR, &signalChr, sizeof(signalChr))) < 0){
		}
		printf("Child %d: run\n", i);
		close(pipeR); //close read		
		
		if (i == CHILD_MAX-1) {
			printf("This is last child.\n");
					
		} else {
			printf("TOPIC CHILD %d: %s\n", i+1,buffer);
			printf("child: %d, Publish RED color: %d\n", i, i+1);
			err = mosquitto_publish(mosq, NULL, buffer, 3, "RED", 1, TRUE);
			printf("MQTT status RED CHILD 2: %s\n", mosquitto_strerror(err));			
			sleep(5);
			
			write(pipeW, &signalChr, sizeof(signalChr));
			
			sleep(5);		
			err = mosquitto_publish(mosq, NULL, buffer, 6, "YELLOW", 1, TRUE);		
			sleep(3);
			err = mosquitto_publish(mosq, NULL, buffer, 5, "GREEN", 1, TRUE);
		}
	}
	
	
	
	printf("CHILD %d terminated\n", i+1);
	close(pipeW);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return 0;
}