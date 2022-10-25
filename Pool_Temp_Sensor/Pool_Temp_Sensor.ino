#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include "Credentials.h"

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 14

// WiFi
WiFiClient wifiClient;
// MQTT Client
PubSubClient mqtt;
// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);  
// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);


// Variables
float currentTemperature = 0.0;
unsigned long timeOfLastTempUpdate = 0;
unsigned long tempUpdateInterval = 3600000; // 1 Hour.
unsigned long timeOfLastReboot = 0;
unsigned long rebootInterval = 86400000; // 24 Hours.
static char tempInCharArray[5];

void setup() {
    Serial.begin(115200);
  // Start the temperature sensors
  sensors.begin();
  
  // Connect to wifi network
  connectWifi();
  
  // Start the MQTT Client
  mqtt.setClient(wifiClient);
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  connectMqtt();
}

void loop(){
  if(timeHasElapsed(timeOfLastReboot, rebootInterval)) {
    timeOfLastReboot = millis();
    ESP.restart();
  }

  if(WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }

  if (!mqtt.connected()) {
    connectMqtt();
  }
  
  updateTemperature();
}

void connectWifi() {
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(WIFI_NAME);
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.println("Connecting to MQTT...");
    
    if (mqtt.connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD )) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(mqtt.state());
      delay(500);
    }
  }
}

bool timeHasElapsed(unsigned long timeOfLastExe, int delayLength) {
  return (millis() - timeOfLastExe > delayLength);
}

void updateTemperature()
{ 
  sensors.requestTemperatures();
  currentTemperature = sensors.getTempCByIndex(0);
  
  if(timeHasElapsed(timeOfLastTempUpdate, tempUpdateInterval)) {
    dtostrf(currentTemperature, 4, 1, tempInCharArray);
    Serial.print("Reporting: ");
    Serial.println(tempInCharArray);
    mqtt.publish(MQTT_TOPIC, tempInCharArray);
    timeOfLastTempUpdate = millis();
  }
}
