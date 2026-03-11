#include <music_dec.hpp>


void play_music(uint8_t BUZ_PIN) {
    pinMode(BUZ_PIN, OUTPUT);
    int melody[] = { NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4 };
    int durations[] = { 4, 8, 8, 4, 4, 4, 4, 4 };
    int size = sizeof(durations) / sizeof(int);
  
    for (int note = 0; note < size; note++) {
      int duration = 1000 / durations[note];
      tone(BUZ_PIN, melody[note], duration);
      int pauseBetweenNotes = duration * 1.30;
      delay(pauseBetweenNotes);
      noTone(BUZ_PIN);
    }
  }