#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <Ticker.h>
#include "FastLED.h"

#include "CompositeSensor.h"
#include "StatusLED.h"
#include "sensor-utils.h"

#include "secrets.h"
#include "settings.h"

/*************************** Settings *******************************************************
 *  Configuration settings stored in permanent FLASH memory                            
 */
#define WRITE_SETTINGS false

#define CFG_DEVICE_TYPE "ESP32_DEVKITC"
#define CFG_CLIENT_ID 4     // 4=livingroom, 5=bedroom : See Location Scheme for location mapping
#define CFG_PINNED_LED true // set true if the status LED is the long-pin type

Preferences preferences;
String cfg_device_type;
int cfg_client_id;
bool cfg_pinned_led;
/*******************************************************************************************/

#define FIRMWARE_NAME "Environment Sensor"
#define FIRMWARE_SLUG "sensor_environment-ESP32_DEVKITC-arduino"
#define FIRMWARE_VERSION "0.2"
#define REFRESH_MILLIS 10000

String device_id;
String make_device_id(String device_type, int id);

// WiFi / MQTT configuration
IPAddress mqtt_server(192, 168, 1, 30);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Initialise sensors
CompositeSensor mySensor;

// StatusLED
StatusLED statusLED;
CRGB data_colour = CRGB::Green;
CRGB wifi_colour = CRGB::Blue;
CRGB error_colour = CRGB::Red;

// timers
Ticker readings_updater;

void setup_wifi();
void reconnect();
void updateReadings();
void publish_float(int client, char *topic, float val);
void publish_int(int client, char *topic, int val);

void setup()
{
  Serial.begin(115200);
  delay(2000);

  mySensor.begin();

  //preferences
  preferences.begin("sensor", false);

  if (WRITE_SETTINGS)
  {
    cfg_device_type = CFG_DEVICE_TYPE;
    cfg_client_id = CFG_CLIENT_ID;
    cfg_pinned_led = CFG_PINNED_LED;

    preferences.putString("device_type", CFG_DEVICE_TYPE);
    preferences.putInt("client_id", CFG_CLIENT_ID);
    preferences.putBool("pinned_led", CFG_PINNED_LED);
  }
  else
  {
    cfg_device_type = preferences.getString("device_type", CFG_DEVICE_TYPE);
    cfg_client_id = preferences.getInt("client_id", -1);
    cfg_pinned_led = preferences.getBool("pinned_led", false);
  }

  device_id = make_device_id(cfg_device_type, cfg_client_id); // TODO refactor this into printBanner
  utils::printBanner(FIRMWARE_NAME, FIRMWARE_SLUG, FIRMWARE_VERSION, device_id.c_str());

  statusLED.drivers(cfg_pinned_led);
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
}

unsigned int cnt = 0;
bool state = true;
void loop()
{
  if (!mqttClient.connected())
    reconnect();

  mqttClient.loop();

  updateReadings();

  delay(REFRESH_MILLIS);
}

/** Callback function for main loop to update and publish readings 
*/
void updateReadings()
{
  CompositeSensor::SensorReadings readings = mySensor.readSensors();

  publish_float(cfg_client_id, "data/temperature", readings.temp);
  publish_int(cfg_client_id, "data/humidity", readings.humidity);
  publish_int(cfg_client_id, "data/co2", readings.co2);

  // statusLED.flash(data_colour, 50);
  statusLED.colour(readings.temp, 15.0, 25.0);

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
    statusLED.flash(wifi_colour, 50);
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
    statusLED.flash(error_colour, 50);

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