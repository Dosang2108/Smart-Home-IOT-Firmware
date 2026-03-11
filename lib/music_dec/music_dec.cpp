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

void play_beep(uint8_t BUZ_PIN) {
    tone(BUZ_PIN, NOTE_C5, 100);
    delay(120);
    noTone(BUZ_PIN);
}

void play_success_sound(uint8_t BUZ_PIN) {
    tone(BUZ_PIN, NOTE_C5, 150);
    delay(170);
    tone(BUZ_PIN, NOTE_E5, 150);
    delay(170);
    tone(BUZ_PIN, NOTE_G5, 200);
    delay(220);
    noTone(BUZ_PIN);
}

void play_error_sound(uint8_t BUZ_PIN) {
    tone(BUZ_PIN, NOTE_A3, 300);
    delay(350);
    tone(BUZ_PIN, NOTE_A3, 300);
    delay(350);
    noTone(BUZ_PIN);
}