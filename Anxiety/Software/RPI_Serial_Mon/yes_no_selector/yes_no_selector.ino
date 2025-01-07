#include "Keyboard.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

const int yPin = 2;     // Pin for 'y'
const int nPin = 3;     // Pin for 'N'
const int killPin = 4;  // Pin for kill command

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
    Keyboard.write(KEY_RETURN);
    delay(200);  // Debounce delay
    while (digitalRead(yPin) == LOW)
      ;  // Wait for button release
  }

  // Check for 'N' button
  if (digitalRead(nPin) == LOW) {
    Keyboard.print('N');
    Keyboard.write(KEY_RETURN);
    delay(200);  // Debounce delay
    while (digitalRead(nPin) == LOW)
      ;  // Wait for button release
  }

  // Check for "exit curr screen session" command button
  if (digitalRead(killPin) == LOW) {
    /**
    * To Kill screen session from inside the screen session, 
    * we do the following:
    *   - Press 'Ctrl-A' followed by '\'
    *   - You'll see "Really kill all windows [y/n]?"
    *   - Press 'y' to confirm
    */

    // Send Ctrl-A
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press('a');
    Keyboard.releaseAll();
    delay(100);  // Small delay between key combinations

    // Send backslash
    Keyboard.print("\\");
    delay(100);

    // Send 'y' to confirm
    Keyboard.print("y");

    delay(200);  // Debounce delay
    while (digitalRead(killPin) == LOW)
      ;  // Wait for button release
  }
}