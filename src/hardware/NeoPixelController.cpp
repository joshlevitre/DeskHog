#include "hardware/NeoPixelController.h"
#include <Arduino.h> // For pinMode, digitalWrite, delay, PI, sin, constrain, millis
// FastLED.h is included via NeoPixelController.h

NeoPixelController::NeoPixelController()
    // No explicit member initializer for leds array or FastLED controller here
    : lastUpdate(0),
      breathPhase(0.0f) {
}

void NeoPixelController::begin() {
    // Power on the NeoPixel (board-specific, keep this logic)
    #if defined(NEOPIXEL_POWER) && defined(NEOPIXEL_POWER_ON)
      pinMode(NEOPIXEL_POWER, OUTPUT);
      digitalWrite(NEOPIXEL_POWER, NEOPIXEL_POWER_ON);
      delay(10);
    #elif defined(NEOPIXEL_POWER)
      pinMode(NEOPIXEL_POWER, OUTPUT);
      digitalWrite(NEOPIXEL_POWER, HIGH);
      delay(10);
    #endif

    // Initialize FastLED
    // Using NEOPIXEL_DATA_PIN from NeoPixelController.h
    FastLED.addLeds<WS2812B, NEOPIXEL_DATA_PIN, GRB>(leds, NUM_PIXELS);
    FastLED.setBrightness(255); // Set max brightness initially
    leds[0] = CRGB::Black; // Clear the pixel
    FastLED.show();
}

void NeoPixelController::update() {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate < UPDATE_INTERVAL_MS) {
        return;
    }
    
    lastUpdate = currentTime;
    
    breathPhase += BREATH_SPEED;
    if (breathPhase >= BREATH_CYCLE) {
        breathPhase -= BREATH_CYCLE;
    }
    
    float brightness = (sin(breathPhase) + 1.0f) * 0.5f;
    
    float redVar = sin(breathPhase * 1.1f) * COLOR_VARIANCE;
    float greenVar = sin(breathPhase * 0.9f) * COLOR_VARIANCE;
    float blueVar = sin(breathPhase * 1.2f) * COLOR_VARIANCE;
    
    float red = (brightness + redVar);
    float green = (brightness + greenVar);
    float blue = (brightness + blueVar);
    
    const uint8_t MIN_VALUE = static_cast<uint8_t>(255.0f * 0.05f);
    
    uint8_t finalRed = constrain(static_cast<long>(red * 255), MIN_VALUE, 255);
    uint8_t finalGreen = constrain(static_cast<long>(green * 255), MIN_VALUE, 255);
    uint8_t finalBlue = constrain(static_cast<long>(blue * 255), MIN_VALUE, 255);
    
    leds[0] = CRGB(finalRed, finalGreen, finalBlue); // Set pixel color using FastLED
    FastLED.show(); // Update the strip
}

void NeoPixelController::blinkLight(int times, int delay_ms) {
    CRGB colors[] = {CRGB::Blue, CRGB::DeepPink, CRGB::Orange};
    int num_colors = sizeof(colors) / sizeof(colors[0]);

    for (int i = 0; i < times; ++i) {
        for (int j = 0; j < num_colors; ++j) {
            leds[0] = colors[j]; // Set to current color in sequence
            FastLED.show();
            delay(delay_ms);
            leds[0] = CRGB::Black; // Turn off the light
            FastLED.show();
            delay(delay_ms);
        }
    }
} 