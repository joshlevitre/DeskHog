#include <lvgl.h>
#include "ui/PomodoroCard.h"
#include "Style.h"
#include "hardware/NeoPixelController.h"
#include <Arduino.h>
#include <string> // Required for std::string

extern NeoPixelController* neoPixelController;

// Define static members for rainbow colors
const uint32_t PomodoroCard::RAINBOW_COLORS[] = {
    0xFF0000,  // Red
    0xFF7F00,  // Orange
    0xFFFF00,  // Yellow
    0x00FF00,  // Green
    0x0000FF,  // Blue
    0x4B0082,  // Indigo
    0x9400D3   // Violet
};
const size_t PomodoroCard::RAINBOW_COLORS_COUNT = sizeof(PomodoroCard::RAINBOW_COLORS) / sizeof(PomodoroCard::RAINBOW_COLORS[0]);

PomodoroCard::PomodoroCard(lv_obj_t* parent)
    : _card(nullptr)
    , _background(nullptr)
    , _label(nullptr)
    , _label_shadow(nullptr)
    , _tally_label(nullptr)
    , _timer(nullptr)
    , _effects_timer(nullptr)
    , _is_running(false)
    , _is_work_mode(true)
    , _remaining_seconds(WORK_TIME)
    , _completed_work_sessions(0)
{
    // Create main card with black background
    _card = lv_obj_create(parent);
    if (!_card) return;
    
    // Set card size and style
    lv_obj_set_width(_card, lv_pct(100));
    lv_obj_set_height(_card, lv_pct(100));
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 5, 0);
    lv_obj_set_style_margin_all(_card, 0, 0);
    
    // Create background container with rounded corners
    _background = lv_obj_create(_card);
    if (!_background) return;
    
    // Make background container fill parent completely
    lv_obj_set_style_radius(_background, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_background, lv_color_hex(0x4A4A4A), 0); // Dark gray background
    lv_obj_set_style_border_width(_background, 0, 0);
    lv_obj_set_style_pad_all(_background, 5, 0);
    
    lv_obj_set_width(_background, lv_pct(100));
    lv_obj_set_height(_background, lv_pct(100));
    
    // Create shadow label (black, 1px offset)
    _label_shadow = lv_label_create(_background);
    if (_label_shadow) {
        lv_obj_set_style_text_font(_label_shadow, Style::largeValueFont(), 0);
        lv_obj_set_style_text_color(_label_shadow, lv_color_black(), 0);
        lv_obj_align(_label_shadow, LV_ALIGN_CENTER, 0, 1);
    }
    
    // Create main label (white, no shadow offset)
    _label = lv_label_create(_background);
    if (_label) {
        lv_obj_set_style_text_font(_label, Style::largeValueFont(), 0);
        lv_obj_set_style_text_color(_label, lv_color_white(), 0);
        lv_obj_align(_label, LV_ALIGN_CENTER, 0, 0);
    }
    
    // Create tally label
    _tally_label = lv_label_create(_card);
    if (_tally_label) {
        lv_obj_set_style_text_font(_tally_label, Style::loudNoisesFont(), 0);
        lv_obj_set_style_text_color(_tally_label, lv_color_white(), 0);
        lv_obj_align(_tally_label, LV_ALIGN_TOP_LEFT, 5, 5);
    }
    
    // Initialize display (including tally)
    updateDisplay();
    updateTallyDisplay();
}

PomodoroCard::~PomodoroCard() {
    if (_timer) {
        lv_timer_del(_timer);
        _timer = nullptr;
    }
    if (_effects_timer) {
        lv_timer_del(_effects_timer);
        _effects_timer = nullptr;
    }
    
    if (isValidObject(_card)) {
        lv_obj_add_flag(_card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_del_async(_card);
        
        _card = nullptr;
        _background = nullptr;
        _label = nullptr;
        _label_shadow = nullptr;
        _tally_label = nullptr;
    }
}

void PomodoroCard::updateDisplay() {
    if (!_label || !_label_shadow) return;
    
    if (_is_running) {
        _remaining_seconds--;
        if (_remaining_seconds <= 0) {
            stopTimer();
            switchMode();

            // Schedule effects to run after a short delay, allowing UI to update
            Serial.println("Timer expired. Scheduling effects.");
            // Ensure no old effects_timer is lingering if stopTimer wasn't perfectly timed (defensive)
            if (_effects_timer) {
                lv_timer_del(_effects_timer);
                _effects_timer = nullptr;
            }
            _effects_timer = lv_timer_create(effects_timer_cb, 100, this); // 100ms delay
            lv_timer_set_repeat_count(_effects_timer, 1); // Make it a one-shot timer
            
            return; // Return from this updateDisplay. The label should now show the reset time.
        }
    }
    
    // Format time as MM:SS
    int minutes = _remaining_seconds / 60;
    int seconds = _remaining_seconds % 60;
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);
    
    lv_label_set_text(_label, time_str);
    lv_label_set_text(_label_shadow, time_str);
}

void PomodoroCard::startTimer() {
    if (_is_running) return;
    
    _is_running = true;
    if (!_timer) {
        _timer = lv_timer_create([](lv_timer_t* timer) {
            auto* card = static_cast<PomodoroCard*>(lv_timer_get_user_data(timer));
            card->updateDisplay();
        }, 1000, this);  // Update every second
    }
}

void PomodoroCard::stopTimer() {
    _is_running = false;
    if (_timer) {
        lv_timer_del(_timer);
        _timer = nullptr;
    }
    if (_effects_timer) { // If effects were scheduled, cancel them
        lv_timer_del(_effects_timer);
        _effects_timer = nullptr;
    }
}

// Custom animation callback to set background color from RAINBOW_COLORS array
void PomodoroCard::anim_set_bg_color_cb(void * var, int32_t v) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(var);
    if (obj && v >= 0 && v < static_cast<int32_t>(RAINBOW_COLORS_COUNT)) {
        lv_obj_set_style_bg_color(obj, lv_color_hex(RAINBOW_COLORS[v]), 0);
    }
}

// Animation ready callback to restore original background color
void PomodoroCard::anim_ready_cb_restore_color(lv_anim_t *a) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(a->var); // Get the animated object
    if (obj) {
        lv_obj_set_style_bg_color(obj, lv_color_hex(0x4A4A4A), 0); // Restore dark gray background
    }
}

void PomodoroCard::flashRainbow() {
    if (!_background) return; // Ensure _background is valid

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _background);
    lv_anim_set_exec_cb(&a, anim_set_bg_color_cb); // Use the custom callback
    lv_anim_set_values(&a, 0, RAINBOW_COLORS_COUNT - 1); // Animate through indices of RAINBOW_COLORS
    lv_anim_set_time(&a, 2000);  // 2 seconds for full rainbow cycle
    lv_anim_set_repeat_count(&a, 2);  // Flash twice
    lv_anim_set_playback_time(&a, 0); // No playback
    lv_anim_set_ready_cb(&a, anim_ready_cb_restore_color); // Set the ready callback
    lv_anim_start(&a);
}

void PomodoroCard::switchMode() {
    if (_is_work_mode) {
        _completed_work_sessions++;
        updateTallyDisplay();
    }
    _is_work_mode = !_is_work_mode;
    _remaining_seconds = _is_work_mode ? WORK_TIME : BREAK_TIME;
    updateDisplay();
}

bool PomodoroCard::handleButtonPress(uint8_t button_index) {
    if (button_index == 1) {  // Center button
        if (_is_running) {
            stopTimer();
        } else {
            startTimer();
        }
        return true;
    }
    return false;
}

bool PomodoroCard::isValidObject(lv_obj_t* obj) const {
    return obj != nullptr && lv_obj_is_valid(obj);
}

void PomodoroCard::effects_timer_cb(lv_timer_t* timer) {
    PomodoroCard* card = static_cast<PomodoroCard*>(lv_timer_get_user_data(timer));
    if (card) {
        // Nullify the pointer as LVGL will delete the one-shot timer automatically
        if (card->_effects_timer == timer) {
             card->_effects_timer = nullptr;
        }
        card->executePostTimerEffects();
    }
}

void PomodoroCard::executePostTimerEffects() {
    Serial.println("Executing post-timer effects. Flashing rainbow.");
    flashRainbow();
    Serial.println("Flashing light.");
    // neoPixelController->blinkLight(3, 500); // This still contains blocking delay() calls
}

void PomodoroCard::updateTallyDisplay() {
    if (!_tally_label) return;

    std::string tally_str = "";
    if (_completed_work_sessions == 0) {
        lv_label_set_text(_tally_label, "");
        return;
    }

    for (int i = 1; i <= _completed_work_sessions; ++i) {
        if (i % 5 == 0) {
            tally_str += "/"; // The 5th mark crosses the previous four implicitly by its position
        } else {
            tally_str += "|";
        }
        // Add a space after a full set of 5, but not if it's the very last mark
        if (i % 5 == 0 && i != _completed_work_sessions) {
             tally_str += " "; 
        }
    }
    lv_label_set_text(_tally_label, tally_str.c_str());
} 