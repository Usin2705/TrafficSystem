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
#define CHECK_WIFI_FLAG BIT_0
#define CHECK_MQTT_FLAG BIT_1

LiquidCrystal_I2C lcd(0x3F, 16, 2);

//WiFiClientSecure espClient;

WiFiClient espClient; //pubsubclient
PubSubClient mqttClient(espClient);


const char* MQTT_TOPIC = "TRAFFIC1";

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	
	lcd.begin(GPIO_NUM_21, GPIO_NUM_22);	
	lcd.backlight();
	lcd.clear();
	tasksEventGrp = xEventGroupCreate();
	xTaskCreate(wifiInit, "wifiInit", 7000, NULL, 1, NULL);
	xTaskCreate(mqttTask, "mqttTask", 7000, NULL, 1, NULL);
}

// the loop function runs over and over again until power down or reset
void loop() {
  
}

void wifiInit(void* parameter) {
	WiFi.mode(WIFI_STA);
	while (1) {
		xEventGroupWaitBits(tasksEventGrp, CHECK_WIFI_FLAG, pdFALSE, pdTRUE, portMAX_DELAY);
		//Check to connect to wifi
		if (!WiFi.isConnected()) {
			WiFi.disconnect();
			vTaskDelay(pdMS_TO_TICKS(500));
			WiFi.begin(ssid, password);
			//WiFi.waitForConnectResult can use this for waiting, but prefer the below method
			lcd.clear();
			for (int i = 0; i < 7; i++) {
				lcd.setCursor(0, 0);
				lcd.printf("Wifi Init");
				lcd.setCursor(i, 1);
				lcd.printf(".");
				vTaskDelay(pdMS_TO_TICKS(1000));
				if (WiFi.isConnected()) {
					esp_wifi_set_ps(WIFI_PS_MODEM);
					Serial.println();
					Serial.println("Connected to WiFi");
					lcd.setCursor(0, 1);
					lcd.printf("Connected!");
					setupTime();

					//Wifi is connected, clear the WIFI bit
					xEventGroupClearBits(tasksEventGrp, CHECK_WIFI_FLAG);
					xEventGroupSetBits(tasksEventGrp, CHECK_MQTT_FLAG);
					break;
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

	while (1) {
		//If not connect to MQTT
		if (!mqttClient.loop()) {
			//First check if we connect to wifi?
			xEventGroupSetBits(tasksEventGrp, CHECK_WIFI_FLAG);
			//Reconnect to MQTT
			reconnect();
		}
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
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("MQTT");
	lcd.setCursor(0, 1);
	lcd.print(payloadstring);
}

void reconnect() {
	// Loop until we're reconnected
	//If Check MQTT flag is set mean: wifi is connected
	xEventGroupWaitBits(tasksEventGrp, CHECK_MQTT_FLAG, pdFALSE, pdTRUE, portMAX_DELAY);
	if (!mqttClient.connected()) {
		lcd.setCursor(0, 0);
		lcd.print("MQTT");
		lcd.setCursor(0, 1);
		lcd.print("Connecting...");
		Serial.print("Attempting MQTT connection...");
		// Create a random mqttClient ID
		String clientId = "ESP8266Client-";
		// This random 4 chars for client ID
		clientId += String(random(0xffff), HEX);
		Serial.println(clientId);

		if (mqttClient.connect(clientId.c_str())) {
			Serial.println("connected");
			lcd.setCursor(0, 1);
			lcd.print("Connected     ");
			//Subscribe to topic
			mqttClient.subscribe(MQTT_TOPIC);
			xEventGroupClearBits(tasksEventGrp, CHECK_MQTT_FLAG);
		}
		else {
			Serial.print("failed, rc=");
			Serial.println(mqttClient.state());
		}

	}

}
