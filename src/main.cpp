#define BLYNK_PRINT Serial
#define BLYNK_NO_FANCY_LOGO

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <Wire.h>
#include "FastLED.h"

#include "KWConfig.h"
#include "CompositeSensor.h"
#include "StatusLED.h"

#include "secrets.h"

/*************************** Settings *******************************************************
 *  Configuration settings stored in permanent FLASH memory                            
 */
#define WRITE_SETTINGS false

#define CFG_CLIENT_ID 4     // 4=livingroom, 5=bedroom : See Location Scheme for location mapping
#define CFG_PINNED_LED true // set true if the status LED is the long-pin type

Preferences preferences;
int cfg_client_id;
bool cfg_pinned_led;
/*******************************************************************************************/

// MQTT configuration
IPAddress mqtt_server(192, 168, 1, 30);
WiFiClient espClient;
PubSubClient mqttClient(espClient);
char info_topic[20];
char temp_topic[20];
char humidity_topic[20];
char co2_topic[20];

// displayable measures
float temperature = 0.0;
int humidity = 0;
int co2 = 0;

// sensor readings refresh time (millis)
int sensor_refresh_millis = KW_DEFAULT_REFRESH_MILLIS;

// StatusLED
StatusLED statusLED;
CRGB data_colour = CRGB::White;
CRGB wifi_colour = CRGB::Blue;
CRGB error_colour = CRGB::Red;
float status_led_brightness = KW_DEFAULT_STATUS_LED_BRIGHTNESS;

// timers
BlynkTimer update_readings_timer;
BlynkTimer breather_timer;
BlynkTimer led_blink_timer;

// chip id
char chip_id[20];

// Initialise sensors
CompositeSensor mySensor;
CompositeSensor::SensorReadings readings;

void logo(char *title, char *version, char *type);
void generate_chip_id();
void reconnect_mqtt();
void update_readings();
void make_topic(char *buf, int client_id, char *topic);
void colourChange();
void publish_float(char *topic, float val);
void publish_int(char *topic, int val);

void setup()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  // pinMode(LED_RED, OUTPUT);
  // pinMode(LED_BLUE, OUTPUT);
  // digitalWrite(LED_BLUE, LOW);
  // digitalWrite(LED_RED, LOW);

  Serial.begin(115200);
  delay(2000);

  mySensor.begin();

  //preferences **************************************************************************
  preferences.begin("sensor", false);

  if (WRITE_SETTINGS)
  {
    cfg_client_id = CFG_CLIENT_ID;
    cfg_pinned_led = CFG_PINNED_LED;

    preferences.putInt("client_id", CFG_CLIENT_ID);
    preferences.putBool("pinned_led", CFG_PINNED_LED);
  }
  else
  {
    cfg_client_id = preferences.getInt("client_id", -1);
    cfg_pinned_led = preferences.getBool("pinned_led", false);
  }
  // preferences ************************************************************************

  logo(FIRMWARE_NAME, FIRMWARE_VERSION, DEVICE_TYPE);

  generate_chip_id();
  BLYNK_LOG("Chip ID: %s", chip_id);

  statusLED.drivers(cfg_pinned_led);

  // setup MQTT
  mqttClient.setServer(mqtt_server, 1883);
  make_topic(info_topic, cfg_client_id, "info/status");
  make_topic(co2_topic, cfg_client_id, "data/co2");
  make_topic(temp_topic, cfg_client_id, "data/temperature");
  make_topic(humidity_topic, cfg_client_id, "data/humidity");

  // configure timers
  update_readings_timer.setInterval(KW_DEFAULT_REFRESH_MILLIS, update_readings);
  breather_timer.setInterval(1L, colourChange);

  Blynk.begin(BLYNK_AUTH, SSID, PASSWD);
  digitalWrite(LED_BLUE, HIGH);
}

unsigned int cnt = 0;
bool state = true;

void loop()
{

  Blynk.run();
  update_readings_timer.run();
  breather_timer.run();

  mqttClient.loop();
}

void colourChange()
{
  statusLED.colour(temperature, KW_DEFAULT_TEMPERATURE_MIN, KW_DEFAULT_TEMPERATURE_MAX);
}

void generate_chip_id()
{
  uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
  uint16_t chip = (uint16_t)(chipid >> 32);
  sprintf(chip_id, "ESP32-%04X-%08X", chip, (uint32_t)chipid);
}

void logo(char *title, char *version, char *type)
{
  char strap_line[200];
  sprintf(strap_line, "                  |___/  %s v%s on %s", title, version, type);

  Serial.println("  _  __ _                                                _ ");
  Serial.println(" | |/ /(_) _ __    __ _  ___ __      __ ___    ___    __| |");
  Serial.println(" | ' / | || '_ \\  / _` |/ __|\\ \\ /\\ / // _ \\  / _ \\  / _` |");
  Serial.println(" | . \\ | || | | || (_| |\\__ \\ \\ V  V /| (_) || (_) || (_| |");
  Serial.println(" |_|\\_\\|_||_| |_| \\__, ||___/  \\_/\\_/  \\___/  \\___/  \\__,_|");
  Serial.println(strap_line);
  Serial.println();
}

/** Callback function for main loop to update and publish readings 
*/
void update_readings()
{
  readings = mySensor.readSensors();

  if (!mqttClient.connected())
    reconnect_mqtt();

  temperature = readings.temp;
  humidity = readings.humidity;
  co2 = readings.co2;

  publish_float(temp_topic, readings.temp);
  publish_int(humidity_topic, readings.humidity);
  publish_int(co2_topic, readings.co2);

  BLYNK_LOG("T:%.1f, H:%d%, CO2:%d", temperature, humidity, co2);
}

String make_device_id(String device_type, int id)
{
  return device_type + "-" + id;
}

/************** WiFi / MQTT  *******************************************************************/

void reconnect_mqtt()
{
  if (!mqttClient.connected())
  {
    // statusLED.flash(error_colour, 50);

    BLYNK_LOG("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(chip_id))
    {
      BLYNK_LOG("MQTT connected");

      // Once connected, publish an announcement...
      mqttClient.publish(info_topic, "ONLINE");
      BLYNK_LOG("MQTT Published ONLINE to [%s]", info_topic);

      // ... and resubscribe
    }
    else
    {
      BLYNK_LOG("MQTT reconnect failed, rc=%d", mqttClient.state());
    }
  }
}

void make_topic(char *buf, int client_id, char *topic)
{
  sprintf(buf, "%d/%s", client_id, topic);
}

void publish_float(char *topic, float val)
{
  char val_buf[10];
  sprintf(val_buf, "%.1f", val);
  mqttClient.publish(topic, val_buf);
}

void publish_int(char *topic, int val)
{
  char val_buf[10];
  sprintf(val_buf, "%d", val);
  mqttClient.publish(topic, val_buf);
}

/***********************************************************************************************/