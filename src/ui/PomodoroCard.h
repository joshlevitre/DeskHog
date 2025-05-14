#pragma once

#include <lvgl.h>
#include <string>
#include "ui/InputHandler.h"

class PomodoroCard : public InputHandler {
public:
    PomodoroCard(lv_obj_t* parent);
    ~PomodoroCard();

    lv_obj_t* getCard() { return _card; }
    bool handleButtonPress(uint8_t button_index) override;
    void updateDisplay();  // Make public for timer callback

private:
    void startTimer();
    void stopTimer();
    void flashRainbow();
    void switchMode();
    bool isValidObject(lv_obj_t* obj) const;

    // UI elements
    lv_obj_t* _card;
    lv_obj_t* _background;
    lv_obj_t* _label;
    lv_obj_t* _label_shadow;
    lv_timer_t* _timer;

    // State
    bool _is_running;
    bool _is_work_mode;  // true for work (25min), false for break (5min)
    int _remaining_seconds;
    static constexpr int WORK_TIME = 25 * 60;    // 25 minutes in seconds
    static constexpr int BREAK_TIME = 5 * 60;    // 5 minutes in seconds
}; 