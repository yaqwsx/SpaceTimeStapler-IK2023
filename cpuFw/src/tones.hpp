#pragma once
#include <Arduino.h>
#include "buzzer.hpp"

Buzzer buzzer;

struct Tones {
    void step() {
        if ( _busy )
            return;
        _busy = true;
        buzzer.beep( DAC_CHANNEL_2, 1000, 0b00 );
        delay( 50 );
        buzzer.stop( DAC_CHANNEL_2 );
        _busy = false;
    }

    void next() {
        if ( _busy )
            return;
        _busy = true;
        buzzer.beep( DAC_CHANNEL_2, 500, 0b00 );
        delay( 100 );
        buzzer.stop( DAC_CHANNEL_2 );
        _busy = false;
    }

    void cap() {
         if ( _busy )
            return;
        _busy = true;
        buzzer.beep( DAC_CHANNEL_2, 1500, 0b00 );
        delay( 15 );
        buzzer.beep( DAC_CHANNEL_2, 2000, 0b00 );
        delay( 15 );
        buzzer.beep( DAC_CHANNEL_2, 2500, 0b00 );
        delay( 15 );
        buzzer.stop( DAC_CHANNEL_2 );
        _busy = false;
    }
    bool _busy;
};
Tones tones;

void fuckup() {
    for ( int i = 0; i != 3; i++ ) {
        for ( int i = 440; i < 4000; i *= 2 ) {
            buzzer.beep( DAC_CHANNEL_2, i, 0 );
            delay( 60 );
        }
        buzzer.stop( DAC_CHANNEL_2 );
    }
}

void famfare() {
    buzzer.beep( DAC_CHANNEL_2, 220, 0 );
    delay( 250 );
    buzzer.beep( DAC_CHANNEL_2, 440, 0 );
    delay( 250 );
    buzzer.beep( DAC_CHANNEL_2, 880, 0 );
    delay( 250 );
    buzzer.beep( DAC_CHANNEL_2, 440, 0 );
    delay( 250 );
    buzzer.beep( DAC_CHANNEL_2, 220, 0 );
    buzzer.stop( DAC_CHANNEL_2 );
};
