#pragma once

#include <FastLED.h>

// Define the data pin for the NeoPixel. 
// This should match PIN_NEOPIXEL from pins_arduino.h for the board (33 for Adafruit ESP32-S3 Reverse TFT)
#ifndef NEOPIXEL_DATA_PIN // Allow override from build flags if needed
  #define NEOPIXEL_DATA_PIN 33 
#endif

class NeoPixelController {
public:
    NeoPixelController();
    void begin();
    void update();
    void blinkLight(int times, int delay_ms);

private:
    static constexpr uint8_t NUM_PIXELS = 1;
    static constexpr uint16_t UPDATE_INTERVAL_MS = 16;  // ~60fps
    static constexpr float BREATH_SPEED = 0.0167f * .75f ;
    static constexpr float BREATH_CYCLE = 2.0f * PI;
    static constexpr float COLOR_VARIANCE = 0.2f;
    // static constexpr uint8_t DATA_PIN = 33; // Explicitly define data pin for FastLED

    CRGB leds[NUM_PIXELS]; // Changed from Adafruit_NeoPixel pixel;
    unsigned long lastUpdate;
    float breathPhase;
}; 