#include "Keyboard.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

const int yPin = 2;      // Pin for 'y'
const int nPin = 3;      // Pin for 'N'
const int killPin = 4;   // Pin for kill command

void setup() {
  // Set pins as inputs with internal pullup resistors
  pinMode(yPin, INPUT_PULLUP);
  pinMode(nPin, INPUT_PULLUP);
  pinMode(killPin, INPUT_PULLUP);

  // Initialize keyboard
  Keyboard.begin();
}

void loop() {
  // Check for 'y' button
  if (digitalRead(yPin) == LOW) {
    Keyboard.print('y');
    delay(200);  // Debounce delay
    while(digitalRead(yPin) == LOW);  // Wait for button release
  }

  // Check for 'N' button
  if (digitalRead(nPin) == LOW) {
    Keyboard.print('N');
    delay(200);  // Debounce delay
    while(digitalRead(nPin) == LOW);  // Wait for button release
  }

  // Check for kill command button
  if (digitalRead(killPin) == LOW) {
    Keyboard.println("sudo killall screen && clear");
    delay(200);  // Debounce delay
    while(digitalRead(killPin) == LOW);  // Wait for button release
  }
}