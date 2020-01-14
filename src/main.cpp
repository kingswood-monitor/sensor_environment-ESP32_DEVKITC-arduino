#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include "FastLED.h"

#include "CompositeSensor.h"
#include "sensor-utils.h"

#include "secrets.h"
#include "settings.h"

/*************************** Settings *******************************************************
 *  Configuration settings stored in permanent FLASH memory                            
 */
Preferences preferences;
String device_type;
int client_id;

#define WRITE_SETTINGS false
#define DEVICE_TYPE "ESP32_DEVKITC"
#define CLIENT_ID 5 // See Location Scheme for location mapping
/*******************************************************************************************/

#define FIRMWARE_NAME "Environment Sensor"
#define FIRMWARE_SLUG "sensor_environment-ESP32_DEVKITC-arduino"
#define FIRMWARE_VERSION "0.2"
#define REFRESH_MILLIS 1000

String device_id;
String make_device_id(String device_type, int id);

// WiFi / MQTT configuration
IPAddress mqtt_server(192, 168, 1, 30);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Initialise sensors
CompositeSensor mySensor;

// FastLED
#define DATA_PIN 18
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

void setup_wifi();
void reconnect();
void updateReadings();
void publish_float(int client, char *topic, float val);
void publish_int(int client, char *topic, int val);

void setup()
{
  Serial.begin(115200);
  delay(2000);

  FastLED.addLeds<WS2812, DATA_PIN>(leds, NUM_LEDS);

  //preferences
  preferences.begin("sensor", false);

  if (WRITE_SETTINGS)
  {
    device_type = DEVICE_TYPE;
    client_id = CLIENT_ID;
    preferences.putString("device_type", DEVICE_TYPE);
    preferences.putInt("client_id", CLIENT_ID);
  }
  else
  {
    device_type = preferences.getString("device_type", DEVICE_TYPE);
    client_id = preferences.getInt("client_id", -1);
  }

  device_id = make_device_id(device_type, client_id);
  utils::printBanner(FIRMWARE_NAME, FIRMWARE_SLUG, FIRMWARE_VERSION, device_id.c_str());

  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);

  mySensor.begin();
}

unsigned int cnt = 0;
bool state = true;
void loop()
{
  leds[0] = CRGB::Green;
  FastLED.show();
  delay(30);
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(30);

  if (!mqttClient.connected())
    reconnect();

  mqttClient.loop();

  updateReadings();

  delay(REFRESH_MILLIS);
}

/** Callback funciton for main loop to update and publish readings 
*/
void updateReadings()
{
  CompositeSensor::SensorReadings readings = mySensor.readSensors();

  publish_float(client_id, "data/temperature", readings.temp);
  publish_int(client_id, "data/humidity", readings.humidity);
  publish_int(client_id, "data/co2", readings.co2);

  char buf[100];
  sprintf(buf, "T:%.1f, H:%.0f%, CO2:%d", readings.temp, readings.humidity, readings.co2);
  Serial.println(buf);
}

String make_device_id(String device_type, int id)
{
  return device_type + "-" + id;
}

/************** WiFi / MQTT  *******************************************************************/

void setup_wifi()
{

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(device_id.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publish_float(int client, char *topic, float val)
{
  char val_buf[10];
  char topic_buf[20];

  sprintf(topic_buf, "%d/%s/", client, topic);
  sprintf(val_buf, "%.1f", val);

  mqttClient.publish(topic_buf, val_buf);
}

void publish_int(int client, char *topic, int val)
{
  char val_buf[10];
  char topic_buf[20];

  sprintf(topic_buf, "%d/%s/", client, topic);
  sprintf(val_buf, "%d", val);

  mqttClient.publish(topic_buf, val_buf);
}

/***********************************************************************************************/