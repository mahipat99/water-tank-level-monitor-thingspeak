Battery operated water tank monitor using Wemos D1 mini and ultrasonic sensor data
------






A basic project to get Ultrasonic Sensor readings from the ESP8266 and write them to a channel on ThingSpeak. You can also user other sensors too. For this project, the user will need to set up an account on ThingSpeak, create a new channel, set the field names, get the Channel ID as and the Write API Key.

Currenly reading are taken at interval of 15 min, you can change it by replacing 15 at line 5 with your requrired value. After reading sensor data and sending it to thingspeak it will go back to deep sleep mode

Software Requirements :
[Arduino IDE](https://www.arduino.cc/en/main/software)



Librarie required for this project are :
[NewPingESP8266](https://github.com/jshaw/NewPingESP8266)

Parts used :

* Wemos D1 mini

* TP4056

* 18650 cell

* Solar panel

* Ultra sonic sensors

* 180k resistor

* 10k resistor

* IN5819 Schottky Diode

* BC557B transistor

Ps - You can also power it using any 5V mobile charger too , skip Solar panel

Diagram -

![alt text][logo]

[logo]: https://raw.githubusercontent.com/mahipat99/water-tank-monitor-thingspeak/master/img/water%20tank_final.png "water tank monitor thingspeak"
