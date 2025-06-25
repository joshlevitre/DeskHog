#pragma once

#include <Arduino.h>
#include <Bounce2.h>

// Forward declare the external buttons array
extern Bounce2::Button buttons[];

class Input {
public:
    static const uint8_t BUTTON_DOWN = 0;    // BOOT button - pulled HIGH by default, LOW when pressed
    static const uint8_t BUTTON_CENTER = 1;  //pulled LOW by default, HIGH when pressed
    static const uint8_t BUTTON_UP = 2;      //pulled LOW by default, HIGH when pressed

    static void configureButtons() {
        Serial.println("Input: Configuring buttons...");
        Serial.printf("Input: BUTTON_DOWN pin: %d\n", BUTTON_DOWN);
        Serial.printf("Input: BUTTON_CENTER pin: %d\n", BUTTON_CENTER);
        Serial.printf("Input: BUTTON_UP pin: %d\n", BUTTON_UP);
        
        // BOOT button has built-in pullup, active LOW
        buttons[BUTTON_DOWN].attach(BUTTON_DOWN, INPUT_PULLUP);
        buttons[BUTTON_DOWN].interval(5);
        buttons[BUTTON_DOWN].setPressedState(LOW);
        Serial.println("Input: Configured BUTTON_DOWN with INPUT_PULLUP, active LOW");
        
        // Configure CENTER and UP buttons with pulldown, active HIGH
        buttons[BUTTON_CENTER].attach(BUTTON_CENTER, INPUT_PULLDOWN);
        buttons[BUTTON_CENTER].interval(5);
        buttons[BUTTON_CENTER].setPressedState(HIGH);
        Serial.println("Input: Configured BUTTON_CENTER with INPUT_PULLDOWN, active HIGH");
        
        buttons[BUTTON_UP].attach(BUTTON_UP, INPUT_PULLDOWN);
        buttons[BUTTON_UP].interval(5);
        buttons[BUTTON_UP].setPressedState(HIGH);
        Serial.println("Input: Configured BUTTON_UP with INPUT_PULLDOWN, active HIGH");
        
        Serial.println("Input: Button configuration complete");
    }

    static void update() {
        buttons[BUTTON_DOWN].update();
        buttons[BUTTON_CENTER].update();
        buttons[BUTTON_UP].update();
    }

    static bool isDownPressed() { return buttons[BUTTON_DOWN].pressed(); }
    static bool isCenterPressed() { return buttons[BUTTON_CENTER].pressed(); }
    static bool isUpPressed() { return buttons[BUTTON_UP].pressed(); }

    static bool isDownReleased() { return buttons[BUTTON_DOWN].released(); }
    static bool isCenterReleased() { return buttons[BUTTON_CENTER].released(); }
    static bool isUpReleased() { return buttons[BUTTON_UP].released(); }
};

