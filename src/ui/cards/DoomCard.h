#pragma once

#include "ui/cards/Card.h"
#include <Audio.h>

class DoomCard : public Card {
public:
    DoomCard(lv_obj_t* parent, CardController* controller);
    ~DoomCard() override;

    void init() override;
    void update() override;
    const char* getName() override { return "Doom E1M1"; }

private:
    void setupAudio();
    void startPlayback();
    void stopPlayback();
    void updateUI();

    lv_obj_t* label;
    Audio* audio; // Pointer to the audio object
    bool isPlaying;
    const char* mp3Url = "https://archive.org/download/doom-1993-ost-at-dooms-gate/Doom%20%281993%29%20OST%20-%20At%20Doom%27s%20Gate.mp3"; // Example URL

    // Define I2S pins
    static constexpr int I2S_BCLK = 42;
    static constexpr int I2S_LRC = 40; // WS
    static constexpr int I2S_DOUT = 41;
}; 