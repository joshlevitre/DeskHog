#include <lvgl.h>
#include "ui/PomodoroCard.h"
#include "Style.h"

PomodoroCard::PomodoroCard(lv_obj_t* parent)
    : _card(nullptr)
    , _background(nullptr)
    , _label(nullptr)
    , _label_shadow(nullptr)
    , _timer(nullptr)
    , _is_running(false)
    , _is_work_mode(true)
    , _remaining_seconds(WORK_TIME)
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
        lv_obj_set_style_text_font(_label_shadow, Style::loudNoisesFont(), 0);
        lv_obj_set_style_text_color(_label_shadow, lv_color_black(), 0);
        lv_obj_align(_label_shadow, LV_ALIGN_CENTER, 0, 1);
    }
    
    // Create main label (white, no shadow offset)
    _label = lv_label_create(_background);
    if (_label) {
        lv_obj_set_style_text_font(_label, Style::loudNoisesFont(), 0);
        lv_obj_set_style_text_color(_label, lv_color_white(), 0);
        lv_obj_align(_label, LV_ALIGN_CENTER, 0, 0);
    }
    
    // Initialize display
    updateDisplay();
}

PomodoroCard::~PomodoroCard() {
    if (_timer) {
        lv_timer_del(_timer);
        _timer = nullptr;
    }
    
    if (isValidObject(_card)) {
        lv_obj_add_flag(_card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_del_async(_card);
        
        _card = nullptr;
        _background = nullptr;
        _label = nullptr;
        _label_shadow = nullptr;
    }
}

void PomodoroCard::updateDisplay() {
    if (!_label || !_label_shadow) return;
    
    if (_is_running) {
        _remaining_seconds--;
        if (_remaining_seconds <= 0) {
            stopTimer();
            flashRainbow();
            switchMode();
            return;
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
}

void PomodoroCard::flashRainbow() {
    static const uint32_t colors[] = {
        0xFF0000,  // Red
        0xFF7F00,  // Orange
        0xFFFF00,  // Yellow
        0x00FF00,  // Green
        0x0000FF,  // Blue
        0x4B0082,  // Indigo
        0x9400D3   // Violet
    };
    
    // Create a rainbow animation
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _background);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_bg_color);
    lv_anim_set_values(&a, 0, sizeof(colors)/sizeof(colors[0]) - 1);
    lv_anim_set_time(&a, 2000);  // 2 seconds for full rainbow cycle
    lv_anim_set_repeat_count(&a, 2);  // Flash twice
    lv_anim_set_playback_time(&a, 0);
    lv_anim_start(&a);
}

void PomodoroCard::switchMode() {
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