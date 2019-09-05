# TrafficSystem-F
<b>Metropolia Realtime operating system project 2019</b>  
Metropolia UAS, 2019.  

<p>

<b>Introduction</b>  
The project demo my ability to make a simple realtime operating system in Linux. I need to create a master process, which control other subprocesses.

<b>AWS code - Traffic light system</b>  
Link to the code of master process and subprocesses: https://github.com/Usin2705/TrafficSystem/tree/master/TrafficSystemAWS

The master process can start and also close other subprocess. It also reponsible for closing all subprocesses and logging.
The subprocess control a mqtt publisher, which will send the MQTT command to ESP32 to controll the traffic light. 

While the traffic system is running, all traffic lights will syncronize to make a smooth traffic. The syncronize is done via pipes. The first traffic light (TRAFFIC0) will start first, then send the signal to the 2nd traffic light. Only when receive the signal through pipe, the 2nd traffic light will start. Whenever the 1st traffic light turn from yellow to red, it will again signal the next traffic light.

<b>ESP32 code - Individual traffic light</b>  
Link to the code of ESP32: https://github.com/Usin2705/TrafficSystem/blob/master/TrafficLightsESP32/TrafficLight/TrafficLight/TrafficLight.ino

To change the traffic light number, we need to change the line 48:

```const char* MQTT_TOPIC = "TRAFFIC2"; //Replace this one for the traffic light number```

At the moment, I only have TRAFFIC0, TRAFFIC1 and TRAFFIC2. It's also the topic of MQTT. so each traffic light will listen to different MQTT topic

The ESP32 has 2 main tasks:

- Log into wifi, then try to connect to MQTT broker (which are supposed to run in AWS server 24/24). Then listen to it's MQTT topic and turn on the correct light, or turn off the traffic light.
- If the broker is down, due to no internet connection, AWS server is turned off or MQTT broker is turned off, then the traffic light will enter "Self Service" mode. In this mode, each traffic light will turn the light following this sequence R - Y - G - Y - R, with Red and Green have 20 seconds countdown, while Yellow have 4 second countdown.
