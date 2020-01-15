#pragma once
/**
 * @file       KWConfig.h
 * @author     Richard Lyon
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2020 Richard Lyon
 * @date       Jan 2020
 * @brief      Configuration of different aspects of the library
 
 */

#define FIRMWARE_NAME "Environment Sensor"
#define FIRMWARE_VERSION "0.21"
#define DEVICE_TYPE "ESP32"

// I2C pins
#define I2C_SDA 21
#define I2C_SCL 22

// FastLED
#define DATA_PIN 18
#define NUM_LEDS 1

// Status pins
#define LED_RED 16
#define LED_BLUE 18

// default sensor update period (millis)
#define KW_DEFAULT_REFRESH_MILLIS 10000

// temperature defaults
#define KW_DEFAULT_TEMPERATURE_MIN 18.0
#define KW_DEFAULT_TEMPERATURE_MAX 25.0

// default display brightness (1.0 = Maximum)
#define KW_DEFAULT_STATUS_LED_BRIGHTNESS 1.0
