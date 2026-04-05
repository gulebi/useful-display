#pragma once

#include <Arduino.h>
#include <EncButton.h>
#include <LiquidCrystal_I2C.h>

extern const uint8_t BACKLIGHT_PIN;
extern const int BACKLIGHT_DEFAULT_BRIGHTNESS;

extern LiquidCrystal_I2C lcd;
extern EncButton enc;
extern int brightness;

bool menuIsActive();
bool menuTick();