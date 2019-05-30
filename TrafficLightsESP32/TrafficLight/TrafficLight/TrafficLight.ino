/*
 Name:		TrafficLight.ino
 Created:	30-May-19 23:53:57
 Author:	Usin
*/
#include <LiquidCrystal_I2C.h>

// the setup function runs once when you press reset or power the board
void setup() {
	LiquidCrystal_I2C lcd(0x3F, 16, 2);
	lcd.begin(GPIO_NUM_21, GPIO_NUM_22);	
	lcd.backlight();
	lcd.setCursor(0, 0);
	lcd.printf("Hello World!");
	lcd.setCursor(0, 1);
	lcd.printf("My bad");
}

// the loop function runs over and over again until power down or reset
void loop() {
  
}
