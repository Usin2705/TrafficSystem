#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//-lmosquitto in compile
#include <mosquitto.h> 

#define TRUE 1
#define FALSE 0
#define MQTT_HOST "ec2-13-48-23-39.eu-north-1.compute.amazonaws.com"
#define MQTT_PORT 1883
#define TOPIC_NAME "TRAFFIC"

//https://www.quora.com/What-is-the-difference-between-the-SIGINT-and-SIGTERM-signals-in-Linux-What%E2%80%99s-the-difference-between-the-SIGKILL-and-SIGSTOP-signals

//https://stackoverflow.com/questions/32479261/c-program-using-mosquitto-client-library-not-working
void on_publish(struct mosquitto *mosq, void *userdata, int mid)
{
   //mosquitto_disconnect(mosq);
   printf("MQTT published \n");
}


int main(int argc, char *argv[])	{
	int keepAlive = 10;
	
	struct mosquitto *mosq;	
	
	int err = 0;
	
	
	//Must be called before any other mosquitto functions.
	//This function is not thread safe.
	//NOT THREAD SAFE
	mosquitto_lib_init();
	
	//Create a new mosquitto client instance.
	mosq = mosquitto_new("id", TRUE, NULL);
	
	if (!mosq) {
		perror("Failed to create mosquitto instance");
		return(1);
	}
	
	//Set the publish callback.  This is called when a message initiated with mosquitto_publish has been sent to the broker successfully.
	mosquitto_publish_callback_set(mosq, on_publish);
	
	//Connect to an MQTT broker.
	err = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, keepAlive);
	
	if (err!=MOSQ_ERR_SUCCESS) {
		printf("Error connect to MQTT broker: %s\n", mosquitto_strerror(err));
	}
	
	const char *msg = argv[2];
	
	char buffer[9] = {'\0'};
	sprintf(buffer, "%s%s", TOPIC_NAME, argv[1]);
	
	//Publish a message on a given topic.
	err = mosquitto_publish(mosq, NULL, buffer, strlen(msg), msg, 1, TRUE);
	
	sleep(5);
	
	err = mosquitto_publish(mosq, NULL, buffer, 5, "GREEN", 1, TRUE);
	
	sleep(5);
	
	err = mosquitto_publish(mosq, NULL, buffer, 6, "YELLOW", 1, TRUE);
	
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return 0;
}