/*
 Name:		TrafficLight.ino
 Created:	30-May-19 23:53:57
 Author:	Usin
*/
#include <WiFi.h>
#include <FreeRTOS\event_groups.h>
//#include <WiFiClientSecure.h>
#include <esp_wifi.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include "Password.h"
#include "Utils.h"

EventGroupHandle_t tasksEventGrp;
#define BIT_0	( 1 << 0 ) //No wifi (unblock wifi initialize task)
#define BIT_1	( 1 << 1 ) //No mqtt (unblock mqtt initialize task)
#define BIT_2	( 1 << 2 ) //No server (unblock self running task)

#define CHECK_WIFI_FLAG BIT_0
#define CHECK_MQTT_FLAG BIT_1
#define SELF_RUN_FLAG BIT_2

LiquidCrystal_I2C lcd(0x3F, 16, 2);

SemaphoreHandle_t mutex;

//WiFiClientSecure espClient;

WiFiClient espClient; //pubsubclient
PubSubClient mqttClient(espClient);

const int redPin = GPIO_NUM_32;
const int yelPin = GPIO_NUM_33;
const int grePin = GPIO_NUM_25;

const int SERVER_TIMEOUT = 60;	//Default  = 60

const int RED_STATE = 0;
const int YELLOW_STATE = 1;
const int GREEN_STATE = 2;
const int ALL_STATE = 3;
const int NONE_STATE = 4;

const char* MQTT_TOPIC = "TRAFFIC2"; //Replace this one for the traffic light number

int isServerWorking = true;


//BUG: AFTER A LONG TIME OF SELF RUN, LCD STOP WORKING (NOT PRINT OUT ANYTHING) (MAYBE THE LCD IS BROKEN SOMEHOW)
//PROBABLY DUE TO WRITING TO LCD WAS INTERUPTED BY OTHER TASK WRITING TO THE SAME LCD
//SHOULD BLOCK THE LCD WRITING WITH SOME MUTEX?? (TRIED, NOT WORK)

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	
	lcd.begin(GPIO_NUM_21, GPIO_NUM_22);	
	lcd.backlight();
	lcd.clear();

	// setup digital output pin
	pinMode(redPin, OUTPUT);
	pinMode(yelPin, OUTPUT);
	pinMode(grePin, OUTPUT);

	//Must create before use
	mutex = xSemaphoreCreateMutex();

	tasksEventGrp = xEventGroupCreate();
	xTaskCreate(wifiInit, "wifiInit", 7000, NULL, 1, NULL);
	xTaskCreate(mqttTask, "mqttTask", 7000, NULL, 1, NULL);
	xTaskCreate(selfService, "selfService", 7000, NULL, 1, NULL);
	/* Start the scheduler */
	//We don't need this to start the tasks :-/
	//vTaskStartScheduler();
}

// the loop function runs over and over again until power down or reset
void loop() {
  
}

void wifiInit(void* parameter) {
	WiFi.mode(WIFI_STA);
	TickType_t lastTime = xTaskGetTickCount();

	while (1) {
		xEventGroupWaitBits(tasksEventGrp, CHECK_WIFI_FLAG, pdFALSE, pdTRUE, portMAX_DELAY);
		//Check to connect to wifi
		if (!WiFi.isConnected()) {
			WiFi.disconnect();
			vTaskDelay(pdMS_TO_TICKS(500));
			WiFi.begin(ssid, password);
			//WiFi.waitForConnectResult can use this for waiting, but prefer the below method
			
			for (int i = 0; i < 7; i++) {

				if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
					lcd.setCursor(0, 1);
					lcd.printf("Wifi");
					lcd.setCursor(i + 4, 1);
					lcd.printf(".");
					xSemaphoreGive(mutex);
				}

				vTaskDelay(pdMS_TO_TICKS(1000));
				if (WiFi.isConnected()) {
					esp_wifi_set_ps(WIFI_PS_MODEM);
					Serial.println();
					Serial.println("Connected to WiFi");
					if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
						lcd.setCursor(5, 1);
						lcd.printf("Connected!");						
						xSemaphoreGive(mutex);
					}
					
					setupTime();
					//Wifi is connected, clear the WIFI bit
					xEventGroupClearBits(tasksEventGrp, CHECK_WIFI_FLAG);
					xEventGroupSetBits(tasksEventGrp, CHECK_MQTT_FLAG);
					
					lastTime = xTaskGetTickCount();
					xEventGroupClearBits(tasksEventGrp, SELF_RUN_FLAG);

					break;
				}
				else {
					if (((xTaskGetTickCount() - lastTime) / xPortGetTickRateHz()) >= SERVER_TIMEOUT) {
						xEventGroupSetBits(tasksEventGrp, SELF_RUN_FLAG);
					}
				}
			}
		} else {
			//Wifi is connected, clear the WIFI bit
			xEventGroupClearBits(tasksEventGrp, CHECK_WIFI_FLAG);
			xEventGroupSetBits(tasksEventGrp, CHECK_MQTT_FLAG);
		}

	}
}

void mqttTask(void* parameter) {

	//I guess to use secure MQTT client, you'll need to set up something inside AWS
	//I tested with iot.eclipse.org:8883 (secured server) and it work
	//However, when test with AWS it did not (I also has secure port at 1884)
	//Maybe I did not set it up properly, or set it wrong somehow.
	//espClient.setCACert(test_root_ca);
	mqttClient.setServer(mqtt_server, 1883);
	mqttClient.setCallback(mqttCallback);

	TickType_t lastTime = xTaskGetTickCount();
	while (1) {
		//If not connect to MQTT
		if (!mqttClient.loop()) {
			//First check if we connect to wifi?
			xEventGroupSetBits(tasksEventGrp, CHECK_WIFI_FLAG);
			//Reconnect to MQTT
			reconnect();
			
			//If MQTT is down for more than 60s
			if (((xTaskGetTickCount() - lastTime) / xPortGetTickRateHz()) >= SERVER_TIMEOUT) {
				xEventGroupSetBits(tasksEventGrp, SELF_RUN_FLAG);
			}
		}

		else {			
			lastTime = xTaskGetTickCount();
			xEventGroupClearBits(tasksEventGrp, SELF_RUN_FLAG);
		}
	}
}

void selfService(void* parameter) {
	TickType_t lastTime = xTaskGetTickCount();

	int state = YELLOW_STATE;
	int timeLapse = 0;
	int count = 0;
	int increment = -1;
	while (1) {
		//waiting until self run (server are down) flag are set
		xEventGroupWaitBits(tasksEventGrp, SELF_RUN_FLAG, pdFALSE, pdTRUE, portMAX_DELAY);
		
		if (isServerWorking) {
			turnLightOn(ALL_STATE);
			vTaskDelay(pdMS_TO_TICKS(500));
			turnLightOn(NONE_STATE);
			vTaskDelay(pdMS_TO_TICKS(500));
			turnLightOn(ALL_STATE);
			vTaskDelay(pdMS_TO_TICKS(500));
			turnLightOn(NONE_STATE);

			lastTime = xTaskGetTickCount();
			state = YELLOW_STATE;
			timeLapse = 0;
			count = 0;
			increment = -1;			

			isServerWorking = false;
		}

		if (((xTaskGetTickCount() - lastTime) / xPortGetTickRateHz()) >= timeLapse) {
			count = 0;
			
			if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
				lcd.clear();
				lcd.setCursor(0, 0);
				lcd.print("SERVER ERROR!");
				xSemaphoreGive(mutex);
			}

			state = state + increment;

			//Swith the increment sign (to make sure it go R-Y-G-Y-R
			if ((state == GREEN_STATE) || (state == RED_STATE)) increment = increment*-1;
			turnLightOn(state);
			lastTime = xTaskGetTickCount();
		}

		timeLapse = state == 1 ? 4 : 20;
		count++;	

		if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
			lcd.setCursor(14, 0);
			lcd.printf("%02d", timeLapse - count / 2);
			xSemaphoreGive(mutex);
		}

		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

//https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
void mqttCallback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	char payloadstring[MQTT_MAX_PACKET_SIZE + 1];
	//for 4096 max can only send approximate 4086 byte back (sometime more sometime less), 
	//maybe should keep payload less than 4000 bytes(safety first)
	memset(payloadstring, '\0', sizeof(payloadstring)); //is this optimal
	memcpy(payloadstring, payload, length);

	//To publish the mqtt data + the traffic light number for debug
	char buf[MQTT_MAX_PACKET_SIZE + 1 + 9];
	sprintf(buf, "%s %s", MQTT_TOPIC, payloadstring);

	mqttClient.publish("Status", buf);
	if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("MQTT");
		lcd.setCursor(0, 1);
		lcd.print(payloadstring);
		xSemaphoreGive(mutex);
	}
	char red[] = "RED";
	char yellow[] = "YELLOW";
	char green[] = "GREEN";
	
	if (memcmp(payloadstring, red, sizeof(red))==0) {
		turnLightOn(RED_STATE);
	}
	else if (memcmp(payloadstring, yellow, sizeof(yellow)) == 0) {
		turnLightOn(YELLOW_STATE);

	} 
	else if (memcmp(payloadstring, green, sizeof(green)) == 0) {
		turnLightOn(GREEN_STATE);
	}
}

void reconnect() {
	// Loop until we're reconnected
	//If Check MQTT flag is set mean: wifi is connected
	xEventGroupWaitBits(tasksEventGrp, CHECK_MQTT_FLAG, pdFALSE, pdTRUE, portMAX_DELAY);
	if (!mqttClient.connected()) {
		/*
		lcd.setCursor(0, 0);
		lcd.print("MQTT");
		lcd.setCursor(0, 1);
		lcd.print("Connecting...");
		*/
		Serial.print("Attempting MQTT connection...");
		// Create a random mqttClient ID
		String clientId = "ESP8266Client-";
		// This random 4 chars for client ID
		clientId += String(random(0xffff), HEX);
		Serial.println(clientId);

		if (mqttClient.connect(clientId.c_str())) {
			Serial.println("connected");

			if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
				lcd.clear();
				lcd.setCursor(0, 1);
				lcd.print("MQTT Connected!");
				xSemaphoreGive(mutex);
			}

			//Subscribe to topic
			mqttClient.subscribe(MQTT_TOPIC);
			xEventGroupClearBits(tasksEventGrp, CHECK_MQTT_FLAG);
			xEventGroupClearBits(tasksEventGrp, SELF_RUN_FLAG);
			isServerWorking = true;
		}
		else {
			Serial.print("failed, rc=");
			Serial.println(mqttClient.state());
		}

	}

}

void turnLightOn(int lightNo) {

	switch (lightNo) {
	
	case RED_STATE:

		digitalWrite(redPin, HIGH);
		digitalWrite(yelPin, LOW);
		digitalWrite(grePin, LOW);
		break;

	case YELLOW_STATE:
		digitalWrite(redPin, LOW);
		digitalWrite(yelPin, HIGH);
		digitalWrite(grePin, LOW);
		break;

	case GREEN_STATE:
		digitalWrite(redPin, LOW);
		digitalWrite(yelPin, LOW);
		digitalWrite(grePin, HIGH);
		break;

	case ALL_STATE:
		digitalWrite(redPin, HIGH);
		digitalWrite(yelPin, HIGH);
		digitalWrite(grePin, HIGH);
		break;

	default:
		digitalWrite(redPin, LOW);
		digitalWrite(yelPin, LOW);
		digitalWrite(grePin, LOW);
		break;
	}
}