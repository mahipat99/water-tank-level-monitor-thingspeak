Water tank monitor

Battery operated water tank monitor using Wemos D1 mini and ultrasonic sensor data

A basic project to get Ultrasonic Sensor readings from the ESP8266 and write them to a channel on ThingSpeak. You can also user other sensors too. For this project, the user will need to set up an account on ThingSpeak, create a new channel, set the field names, get the Channel ID as and the Write API Key.
As per code reading are taken at interval of 15 min, you can change it by replacing 15 at line 5 with your desired value. After reading sensor data and sending it to thingspeak it will go back to deep sleep mode

Software Requirements :
Arduino IDE

Librarie required for this project are :
NewPingESP8266
