#include <ESP8266WiFi.h>
#include <NewPingESP8266.h>

// Time to sleep (in minute)
const int SLEEPTIME = 15;
// Calculate time for debug
uint32_t bootTime, setupTime, WiFiTime;
// Thingspeak API key,
String apiKey = "xyz123abc";
// Host
const char* server = "api.thingspeak.com";
// WiFi credentials
const char* WLAN_SSID = "name";
const char* WLAN_PASSWD  = "pass";

// RTC data for wifi
struct {
  uint32_t crc32;   // 4 bytes
  uint8_t channel;  // 1 byte,   5 in total
  uint8_t bssid[6]; // 6 bytes, 11 in total
  uint8_t ap_mac[6];
} rtcData;

// Static IP details...
IPAddress ip(192, 168, 1, 101);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

WiFiClient client;
//****************Ping sensor stuff******************
#define TRIGGER_PIN  4  // Arduino pin tied to trigger pin on the ultrasonic sensor. orange
#define ECHO_PIN     5  // Arduino pin tied to echo pin on the ultrasonic sensor. yellow
#define MAX_DISTANCE 100 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPingESP8266 sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

int waterAtEmpty = 126; //distance in cm of sensor to water level at empty.
int waterAtFull = 5; //distance in cm from sensor to water level at full.
int dist;
const int T1 = 12;//ping sensor power pin

#define BATT_LEVEL A0

RF_PRE_INIT() {
  bootTime = millis();
}

void setup() {
  setupTime = millis();
  Serial.begin(74880);
  // disable WiFi
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  // delay( 1 );
  delay(500); //to avoid RTC bug

  // Try to read WiFi settings from RTC memory
  bool rtcValid = false;
  if ( ESP.rtcUserMemoryRead( 0, (uint32_t*)&rtcData, sizeof( rtcData ) ) ) {
    // Calculate the CRC of what we just read from RTC memory, but skip the first 4 bytes as that's the checksum itself.
    uint32_t crc = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
    if ( crc == rtcData.crc32 ) {
      rtcValid = true;
    }
  }

  // Do stuff
  delay(10);
  pinMode(T1, OUTPUT);
  digitalWrite(T1, LOW); // put sensor on sleep mode
  pinMode(D0, WAKEUP_PULLUP); // reset
  pinMode(D4, OUTPUT); // Led
  digitalWrite(D4, LOW); //

  //*******Read water level*****
  digitalWrite(T1, HIGH);//turn on ping sensor via transistor
  delay(1000);//warm up ping sensor
  unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
  digitalWrite(T1, LOW);//turn off ping sensor, save power!
  dist = (uS / US_ROUNDTRIP_CM); //raw distance in cm.
  int waterRange = waterAtEmpty - waterAtFull; //difference between full and empty in cm.
  int adjDist = dist - waterAtFull; //water level from full in cm.
  float percent = map(adjDist, waterRange, 0, 0, 100); //Map waterRange to percent.

  // read the input on analog pin 0:
  int sensorValue = analogRead(BATT_LEVEL);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 3.2V):
  double voltage = ((analogRead(BATT_LEVEL)) / 222.60);//This needs to be adjusted for each case to be accurate. Higher value means low voltage // assumes external 180K resistor

  //Switch Radio back On
  WiFi.forceSleepWake();
  delay( 1 );

  // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.persistent( false );

  // Bring up the WiFi connection
  WiFi.mode( WIFI_STA );
  WiFi.config( ip, dns, gateway, subnet );
  //-----------replacement for WiFi.begin
  if ( rtcValid ) {
    // The RTC data was good, make a quick connection
    WiFi.begin( WLAN_SSID, WLAN_PASSWD, rtcData.channel, rtcData.ap_mac, true );
  }
  else {
    // The RTC data was not valid, so make a regular connection
    WiFi.begin( WLAN_SSID, WLAN_PASSWD );
  }

  // Attempt to connect
  int retries = 0;
  int wifiStatus = WiFi.status();
  while ( wifiStatus != WL_CONNECTED ) {
    retries++;
    if ( retries == 100 ) {
      //5s Quick connect is not working, reset WiFi and try regular connection
      WiFi.disconnect();
      delay( 10 );
      WiFi.forceSleepBegin();
      delay( 10 );
      WiFi.forceSleepWake();
      delay( 10 );
      WiFi.begin( WLAN_SSID, WLAN_PASSWD );
    }
    if ( retries == 200 ) {
      // Giving up after 10s and going back to sleep
      WiFi.disconnect( true );
      delay( 1 );
      unsigned long sleeptimer = (SLEEPTIME * 60) * 1000000;
      Serial.println("");
      Serial.println("Failed to connect to Wifi after 10 seconds!");
      Serial.println("ESP8266 in sleep mode for " + String(SLEEPTIME) + " minutes");
      ESP.deepSleep(sleeptimer, WAKE_RF_DISABLED );
      WiFi.mode( WIFI_OFF );
      return; // Not expecting this to be called, the previous call will never return.
    }
    delay( 50 );
    wifiStatus = WiFi.status();
    WiFiTime = millis();
  }

  //Debug info
  Serial.printf("Core: %s/%s\n", ESP.getCoreVersion().c_str(), ESP.getSdkVersion());
  Serial.println(ESP.getResetReason().c_str());
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("bootTime: %d ms setupTime: %d ms WifiTime %d ms\n", bootTime, setupTime, WiFiTime);
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
  // Write current connection info back to RTC
  rtcData.channel = WiFi.channel();
  memcpy( rtcData.ap_mac, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
  rtcData.crc32 = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
  ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcData, sizeof( rtcData ) );


  //Send data to thingspeak
  if (client.connect(server, 80)) { // "184.106.153.149" or api.thingspeak.com
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(percent);
    postStr += "&field2=";
    postStr += String(voltage);
//    postStr += "&field3=";
//    postStr += String(sensorValue);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    client.stop();

    Serial.print("Tank level ");
    Serial.print(percent);
    Serial.print(",A0 sensor Value ");
    Serial.print(sensorValue);
    Serial.print(",Batt voltage ");
    Serial.print(voltage);
    Serial.print("\n");
    Serial.println("Data sent to Thingspeak");
  }
  else {
    Serial.println("No connections available");
  }

  //Back to sleep
  WiFi.disconnect( true );
  delay( 1 );
  Serial.println("Closing connection");
  Serial.println("ESP8266 in sleep mode for " + String(SLEEPTIME) + " minutes");
  unsigned long sleeptimer = (SLEEPTIME * 60) * 1000000;
  ESP.deepSleep(sleeptimer, WAKE_RF_DISABLED );
  //  ESP.deepSleep(ESP.deepSleepMax(), WAKE_RF_DISABLED );
}

void loop() {
}

uint32_t calculateCRC32( const uint8_t *data, size_t length ) {
  uint32_t crc = 0xffffffff;
  while ( length-- ) {
    uint8_t c = *data++;
    for ( uint32_t i = 0x80; i > 0; i >>= 1 ) {
      bool bit = crc & 0x80000000;
      if ( c & i ) {
        bit = !bit;
      }

      crc <<= 1;
      if ( bit ) {
        crc ^= 0x04c11db7;
      }
    }
  }

  return crc;
}
