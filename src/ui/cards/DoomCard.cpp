#include "DoomCard.h"
#include "ui/CardController.h"
#include "Arduino.h"
#include "Log.h"

// Forward declaration for audio status callback
void audio_showstation(const char* info);
void audio_eof_mp3(const char *info); // End of file

DoomCard::DoomCard(lv_obj_t* parent, CardController* controller)
    : Card(parent, controller), label(nullptr), audio(nullptr), isPlaying(false) {
    Log::info("Creating DoomCard");
    // Allocate audio object on the heap
    audio = new Audio(true); // Use PSRAM if available
}

DoomCard::~DoomCard() {
    Log::info("Destroying DoomCard");
    stopPlayback();
    if (audio) {
        delete audio;
        audio = nullptr;
    }
    // LVGL objects associated with the card are destroyed automatically
    // when the parent object (this->card) is deleted by the base Card destructor.
}

void DoomCard::init() {
    Log::info("Initializing DoomCard");
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x8B0000), LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);

    label = lv_label_create(card);
    lv_obj_set_width(label, LV_PCT(100));
    lv_label_set_text(label, "Connecting...");
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);

    setupAudio();
    startPlayback();
}

void DoomCard::update() {
    if (audio && isPlaying) {
        audio->loop();
    }
    // updateUI() can be called from audio callbacks or here if needed
}

void DoomCard::setupAudio() {
    if (!audio) return;

    Log::info("Setting up Audio - I2S Pins: BCLK=%d, LRC=%d, DOUT=%d", I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio->setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    // Set volume (0.0 - 4.0)
    audio->setVolume(21); // Corresponds to 1.0? Library seems to use 0-21 integer scale.
    // Let's try setting it to a reasonable default first, maybe 15.
    audio->setVolume(15); 

    // Assign callback functions
    audio->setConnectionTimeout(5000, 5000); // Connect timeout, data timeout
    audio->setAudioDataStream(nullptr); // Using internal buffer
    audio->setStreamTimeout(true, 10000); // Enable stream timeout, 10 seconds
    // audio->setTone(nullptr);
    // audio->setToneGenerator(nullptr);
    // audio->setAudioPlayListItem(nullptr);
    // audio->setAudioPlayerWrapper(nullptr);
    // audio->setForceMono(false);
    // audio->setBalance(0);
    // audio->setInternalDAC(false); // We are using external I2S
    // audio->setI2SCommFMT_LSB(false); // Standard I2S format

    // Example callbacks (adjust based on actual library functions if needed)
    audio->setStatusPin(nullptr); // No status LED pin
    // Need to find the correct function names for callbacks
    // audio->setCallbackStatus(audio_status_callback); 
    // audio->setCallbackEOF(audio_eof_callback);
    // audio->setCallbackError(audio_error_callback);

    // From library examples, these seem to be the way:
    audio_showstation = [](const char* info){
        Serial.print("STATUS: "); Serial.println(info);
        // Update label if possible/needed
    };
    audio_eof_mp3 = [this](const char *info){
      Serial.print("EOF MP3: "); Serial.println(info);
      this->isPlaying = false;
      lv_label_set_text(this->label, "Playback Finished");
      // Optionally: tell CardController to switch cards
      // this->controller->nextCard(); 
    };

    // Note: Other callbacks like `audio_info`, `audio_id3data` etc. exist
}

void DoomCard::startPlayback() {
    if (!audio || isPlaying) return;

    Log::info("Starting playback from URL: %s", mp3Url);
    if (audio->connecttohost(mp3Url)) {
        isPlaying = true;
        updateUI(); // Set initial state
    } else {
        Log::error("Failed to connect to host: %s", mp3Url);
        lv_label_set_text(label, "Error: Connection failed");
        isPlaying = false;
    }
}

void DoomCard::stopPlayback() {
    if (!audio || !isPlaying) return;

    Log::info("Stopping playback");
    audio->stopSong();
    isPlaying = false;
    updateUI(); // Update UI to stopped state
}

void DoomCard::updateUI() {
    if (!label) return;

    if (isPlaying) {
        // Potentially update based on audio status callbacks
        lv_label_set_text(label, "Playing E1M1...");
    } else {
        // Handled by EOF callback or error in startPlayback
         if (lv_label_get_text(label) == "Connecting...") { // Check if it's still connecting
             lv_label_set_text(label, "Stopped");
         }
    }
}

// Optional: Implement global callback functions if required by the library
// and if lambda captures aren't sufficient/possible.
// void audio_status_callback(const char* status) {
//     Serial.printf("Audio Status: %s\n", status);
// }
// void audio_eof_callback(const char* info) {
//     Serial.printf("Audio EOF: %s\n", info);
//     // Need a way to signal the DoomCard instance here
// }
// void audio_error_callback(const char* error) {
//      Serial.printf("Audio Error: %s\n", error);
//      // Need a way to signal the DoomCard instance here
// } 