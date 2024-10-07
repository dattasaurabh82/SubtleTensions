/*
TBD:
1. Double click forward button, increases speed to move forward.
2. Check the actual pin state of Motor Enable Pin and and then update the LEDs.
*/

// Stepper Motor ctrl pin definitions
#define motorEnablePin 4
#define motorDirPin 5
#define motorPulPin 6

// Button pins definition (Stepper motor Action buttons)
#define btnPinToEnableMotor 2
#define btnPinToReverseMotor 3
#define btnPinToForwardMotor 7

// Limit button pin definitions
#define btnPinFrontLimit 10
#define btnPinBackLimit 16

// LED pins definition
#define motorEnabledLED 8
#define motorDisabledLED 9


// ----------------------------- //
// START OF VARIABLE DEFINITIONS //
// ----------------------------- //
// Button click handling variables
volatile bool enableMotorBtnPressed = false;
volatile unsigned long lastDebounceTimeForEnableMotorBtnPress = 0;
unsigned long enableMotorBtnDebounceDelay = 50;

volatile bool reverseMotorDirBtnPressed = false;
volatile unsigned long lastDebounceTimeForReverseMotorBtnPress = 0;
unsigned long reverseMotorBtnDebounceDelay = 50;

volatile bool forwardMotorDirBtnPressed = false;
volatile unsigned long lastDebounceTimeForForwardMotorBtnPress = 0;
unsigned long forwardMotorBtnDebounceDelay = 50;


// Limit button variables
unsigned long lastDebounceTime[2] = { 0, 0 };
bool lastButtonState[2] = { HIGH, HIGH };
bool buttonState[2] = { HIGH, HIGH };
const unsigned long limitBtnDebounceDelay = 50;

// New variables for limit switch handling
volatile bool movingAwayFromFrontLimit = false;
volatile bool movingAwayFromBackLimit = false;
unsigned long limitReleaseTime = 0;
const unsigned long additionalMoveTime = 1000;  // 1 second of additional movement



// Stepper Motor motion ctrl variables
volatile bool motorEnabled = false;
volatile unsigned long stepSpeedinUS = 500;  // Some initial value (anyways, will get replaced as per action)
unsigned long lastTime = 0;

const unsigned long reverseSpeed = 250;   // 250
const unsigned long forwardSpeed = 2550;  // 2500
const unsigned long limitRetractionSpeed = 100; 

// --------------------------- //
// END OF VARIABLE DEFINITIONS //
// --------------------------- //





// ----------------------------- //
// START OF FUNCTION DEFINITIONS //
// ----------------------------- //
// ISR function to detect if the stepper motor enable/disable toggle button was pressed
void enableMotorCtrlISR() {
  enableMotorBtnPressed = true;
}

// ISR function to detect if the stepper motor is called upon to go in reverse (to fill the syringe)
void reverseMotorDirISR() {
  reverseMotorDirBtnPressed = true;
}

// ISR function to detect if the stepper motor is called upon to go forward in reverse (to fill the syringe)
void forwardMotorDirISR() {
  forwardMotorDirBtnPressed = true;
}

// Utility function to display state of stepper motor (enabled/disabled)
void updateLEDs(volatile bool motorState) {
  digitalWrite(motorEnabledLED, motorState ? HIGH : LOW);
  digitalWrite(motorDisabledLED, motorState ? LOW : HIGH);
}



// Utility functions for handling stepper motor spin
void disableMotor() {
  // If motor was enabled
  if (motorEnabled) {
    // Disable the motor motion
    Serial.println("Disabling Motor...");
    digitalWrite(motorEnablePin, LOW);
    motorEnabled = false;
    updateLEDs(motorEnabled);
    Serial.println("Disabled Motor!");
  }
}

void enableMotor() {
  if (!motorEnabled) {
    // Enable the motor motion
    Serial.println("Enabling Motor...");
    digitalWrite(motorEnablePin, HIGH);
    motorEnabled = true;
    updateLEDs(motorEnabled);
    Serial.println("Enabled Motor!");
  }
}

void reverseMotor(int stepSpeed) {
  // 1. Disable the motor motion
  disableMotor();
  delay(1000);
  // 2. Set the spinning direction anti-clockwise & set the step speed
  Serial.print("Setting motor dir to \"clockwise\" & setting the motor speed by: ");
  Serial.print(stepSpeed);
  Serial.println(" uS");
  digitalWrite(motorDirPin, LOW);
  stepSpeedinUS = stepSpeed;
  // 3. Enable the motor motion
  enableMotor();
}

void forwardMotor(int stepSpeed) {
  // 1. Disable the motor motion
  disableMotor();
  delay(1000);
  // 2. Set the spinning direction anti-clockwise & set the step speed
  Serial.print("Setting motor dir to \"anti-clockwise\" & setting the motor speed by: ");
  Serial.print(stepSpeed);
  Serial.println(" uS");
  digitalWrite(motorDirPin, HIGH);
  stepSpeedinUS = stepSpeed;
  // 3. Enable the motor motion
  enableMotor();
}

void stopAndDisableMotor() {
  digitalWrite(motorEnablePin, LOW);
  motorEnabled = false;
  updateLEDs(motorEnabled);
  Serial.println("Motor stopped and disabled after limit switch action");
}

void stepperMotorUtility() {
  unsigned long currentMicros = micros();
  if (currentMicros - lastTime >= stepSpeedinUS) {
    lastTime = currentMicros;
    digitalWrite(motorPulPin, !digitalRead(motorPulPin));  // Toggle the motor step pin state
  }
}


// Function to determine, in paralell, if and when limit switches are being hit
bool limitPressed(int pin) {
  int buttonIndex = (pin == btnPinFrontLimit) ? 0 : 1;
  bool reading = digitalRead(pin);

  if (reading != lastButtonState[buttonIndex]) {
    lastDebounceTime[buttonIndex] = millis();
  }

  if ((millis() - lastDebounceTime[buttonIndex]) > limitBtnDebounceDelay) {
    if (reading != buttonState[buttonIndex]) {
      buttonState[buttonIndex] = reading;
    }
  }

  lastButtonState[buttonIndex] = reading;
  return buttonState[buttonIndex];
}



void setup() {
  // Disable on board RX TX LEDs
  pinMode(LED_BUILTIN_TX, INPUT);
  pinMode(LED_BUILTIN_RX, INPUT);

  Serial.begin(115200);

  // Declare Button pins & ISR functions
  pinMode(btnPinToEnableMotor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(btnPinToEnableMotor), enableMotorCtrlISR, FALLING);
  pinMode(btnPinToForwardMotor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(btnPinToForwardMotor), forwardMotorDirISR, FALLING);
  pinMode(btnPinToReverseMotor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(btnPinToReverseMotor), reverseMotorDirISR, FALLING);

  // Declare Limit buttons
  pinMode(btnPinFrontLimit, INPUT_PULLUP);
  pinMode(btnPinBackLimit, INPUT_PULLUP);

  // // Signal LED pins & State declaration
  pinMode(motorEnabledLED, OUTPUT);
  pinMode(motorDisabledLED, OUTPUT);

  // Declare Stepper Motor pins as output
  pinMode(motorDirPin, OUTPUT);
  pinMode(motorPulPin, OUTPUT);
  pinMode(motorEnablePin, OUTPUT);

  // At start: set all button vals to false
  enableMotorBtnPressed = false;
  forwardMotorDirBtnPressed = false;
  reverseMotorDirBtnPressed = false;

  // At Start: disable Motor
  motorEnabled = false;
  // motorRunning = true;
  digitalWrite(motorEnablePin, motorEnabled);
  updateLEDs(motorEnabled);
  delay(500);

  Serial.println("System initialized.");
}


void loop() {
  // ------------------------------------------------------------- //
  // Handle Enabling Disabling motor through Action Button Presses //
  // ------------------------------------------------------------- //
  if (enableMotorBtnPressed) {
    unsigned long currTimeAtMotorEnableBtnPress = millis();
    if ((currTimeAtMotorEnableBtnPress - lastDebounceTimeForEnableMotorBtnPress) > enableMotorBtnDebounceDelay) {
      Serial.println("\nEnable/Disable Motor Button Clicked!");

      motorEnabled = !motorEnabled;
      digitalWrite(motorEnablePin, motorEnabled);
      // [TBD]: Check the actual pin state and then update the LEDs
      updateLEDs(motorEnabled);

      Serial.print("Requested Motor State:\t");
      if (motorEnabled) {
        Serial.println("ENABLED\n");
      } else {
        Serial.println("DISABLED\n");
      }

      lastDebounceTimeForEnableMotorBtnPress = currTimeAtMotorEnableBtnPress;
    }
    enableMotorBtnPressed = false;
  }


  // -------------------------------------------------------------------- //
  // On Forward Action button Press, handle Motor Reversing Functionality //
  // -------------------------------------------------------------------- //
  if (forwardMotorDirBtnPressed) {
    Serial.println("\nFORWARD btn pressed");
    forwardMotor(forwardSpeed);  // low step speed in micro-sec
    forwardMotorDirBtnPressed = false;
    delay(1000);
  }

  // --------------------------------------------------------------------- //
  // On Backward Action button Press, handle Motor Reversing Functionality //
  // --------------------------------------------------------------------- //
  if (reverseMotorDirBtnPressed) {
    Serial.println("\nREVERSE btn pressed");
    reverseMotor(reverseSpeed);  // high step speed in micro-sec
    reverseMotorDirBtnPressed = false;
    delay(1000);
  }


  // ----------------------------------------------------------- //
  // Handle Limit Switch hitting functionality logic abstraction //
  // ----------------------------------------------------------- //
  bool frontLimitPressed = !limitPressed(btnPinFrontLimit);
  bool backLimitPressed = !limitPressed(btnPinBackLimit);

  if (frontLimitPressed && !movingAwayFromFrontLimit) {
    Serial.println("\nFront Limit Button Pressed. Reversing!");
    reverseMotor(limitRetractionSpeed);
    movingAwayFromFrontLimit = true;
  } else if (!frontLimitPressed && movingAwayFromFrontLimit) {
    if (limitReleaseTime == 0) {
      limitReleaseTime = millis();
    } else if (millis() - limitReleaseTime > additionalMoveTime) {
      stopAndDisableMotor();
      movingAwayFromFrontLimit = false;
      limitReleaseTime = 0;
    }
  }

  if (backLimitPressed && !movingAwayFromBackLimit) {
    Serial.println("\nBack Limit Button Pressed. Moving Forward!");
    forwardMotor(limitRetractionSpeed);
    movingAwayFromBackLimit = true;
  } else if (!backLimitPressed && movingAwayFromBackLimit) {
    if (limitReleaseTime == 0) {
      limitReleaseTime = millis();
    } else if (millis() - limitReleaseTime > additionalMoveTime) {
      stopAndDisableMotor();
      movingAwayFromBackLimit = false;
      limitReleaseTime = 0;
    }
  }


  // ---------------------------- //
  // Parallel task: Motor Utility //
  // ---------------------------- //
  stepperMotorUtility();
}