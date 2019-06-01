// Utils.h
#ifndef _UTILS_h
#define _UTILS_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif


#endif

#include <Arduino.h>
#include <ArduinoOTA.h> //the setup time function wont work without this


void setupTime();