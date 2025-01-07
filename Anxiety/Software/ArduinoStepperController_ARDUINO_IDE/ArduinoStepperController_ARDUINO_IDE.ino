/**
* @file syringe_pump_control.cpp
* @brief Stepper motor controller for automated syringe pump system
* @details Controls bi-directional stepper motor movement with limit switches,
*          solenoid valves, and status LEDs for a medical syringe pump
* @author Saurabh Datta
* @date Nov 2024
* 
* @note Built for Arduino Pro Micro (ATmega32U4) or Arduino Micro: https://amzn.eu/d/dSED3HR
* @warning Medical device - requires validation before clinical use
*/

/*
TBD:
* Double click forward button, increases speed to move forward.
*/


/** Pin Definitions Section
* @defgroup PinDefinitions Hardware Pin Mappings
* @{
*/
/**
* @brief Stepper Motor Control Pins
* @details Defines pins for enable, direction and pulse control
*/
#define motorEnablePin 4
#define motorDirPin 5
#define motorPulPin 6


/**
* @brief Input Button Pins 
* @details Control buttons and limit switch inputs
*/
// Button pins definition (Stepper motor Action buttons)
#define btnPinToEnableMotor 2
#define btnPinToReverseMotor 3
#define btnPinToForwardMotor 7
// Limit button pin definitions
#define btnPinFrontLimit 16
#define btnPinBackLimit 10

/**
* @brief Output Control Pins
* @details LED indicators and solenoid valve controls
* @{
*/
// LED pins definition
#define motorEnabledLED 8   // Non-PWM pin
#define motorDisabledLED 9  // PWM enabled pin
// Solenoid valve pins
#define fillSolenoid 14
#define flowSolenoid 15
/** @} */  // End of pin definitions



/**
* @brief Global State Variables
* @details Tracks system state including:
* - Button debounce timing
* - Motor movement state
* - Limit switch states
* - Movement timing parameters
*/
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
const unsigned long additionalMoveTime = 5000;  // 1 second of additional movement

// Stepper Motor motion ctrl variables
volatile bool motorEnabled = false;
volatile unsigned long stepSpeedinUS = 500;  // Some initial value (anyways, will get replaced as per action)
unsigned long lastTime = 0;

const unsigned long reverseSpeed = 100;  // 100
const unsigned long forwardSpeed = 100;  // 2500
const unsigned long limitRetractionSpeed = 50;




/**
* @brief Button Press ISR Functions
* @details Interrupt handlers for motor control buttons
* @param None
* @return None
*/
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


/**
* @brief Updates status LEDs based on motor state
* @param motorState [in] TRUE: motor enabled, FALSE: motor disabled
* @note Pin 8: Digital output, Pin 9: PWM (brightness: 5)
*/
void updateLEDs(volatile bool motorState) {
  digitalWrite(motorEnabledLED, motorState ? HIGH : LOW);
  // digitalWrite(motorDisabledLED, motorState ? LOW : HIGH);
  analogWrite(motorDisabledLED, motorState ? 0 : 5);
}


/**
* @brief Motor Control Functions
* @details Core functions for motor operation:
* - Enable/disable motor
* - Direction control
* - Speed control
* - Emergency stop handling
*/
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
    delay(10);
    while (digitalRead(motorEnablePin) != HIGH) {
      Serial.println("Motor Enablement Failed!");
      Serial.println("Retrying ...");
      motorEnabled = false;
      digitalWrite(motorEnablePin, HIGH);
      delay(100);
    }
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


/**
* @brief Limit Switch Handler
* @details Processes limit switch inputs with debouncing in a non-blocking fashion
* @param pin Input pin to check
* @return bool TRUE if switch is pressed
*/
// Function to determine, in parallel, if and when limit switches are being hit
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


/**
* @brief Valve Control Functions
* @details Controls solenoid valves for:
* - Syringe filling operation
* - Flow dispensing operation 
*/
void ctrlValveStatesFillSyringe() {
  Serial.println("Closing Flow Valve.");
  digitalWrite(flowSolenoid, HIGH);  // close main outgoing flow valve
  delay(1000);
  Serial.println("Opening Refill Valve.");
  digitalWrite(fillSolenoid, LOW);  // open intake re-fill valve
  delay(1000);
}

void ctrlValveStatesFlowSyringe() {
  Serial.println("Closing Refill Valve.");
  digitalWrite(fillSolenoid, HIGH);  // close intake re-fill valve
  delay(1000);
  Serial.println("Opening Flow Valve.");
  digitalWrite(flowSolenoid, LOW);  // open main outgoing flow valve
  delay(1000);
}



void setup() {
  // Disable on board RX TX LEDs
  pinMode(LED_BUILTIN_TX, INPUT);
  pinMode(LED_BUILTIN_RX, INPUT);

  Serial.begin(115200);
  while (!Serial) {
    ;
  }


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

  // Signal LED pins & State declaration
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
  digitalWrite(motorEnablePin, motorEnabled);
  updateLEDs(motorEnabled);
  delay(500);

  // Solenoid valve ctrl pins
  pinMode(fillSolenoid, OUTPUT);
  pinMode(flowSolenoid, OUTPUT);
  ctrlValveStatesFlowSyringe();

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
    ctrlValveStatesFlowSyringe();  // Close Refill Valve & Open Main Valve
    forwardMotor(forwardSpeed);    // Go forward in low speed (low step speed in micro-sec)
    forwardMotorDirBtnPressed = false;
    delay(1000);
  }

  // --------------------------------------------------------------------- //
  // On Backward Action button Press, handle Motor Reversing Functionality //
  // --------------------------------------------------------------------- //
  if (reverseMotorDirBtnPressed) {
    Serial.println("\nREVERSE btn pressed");
    ctrlValveStatesFillSyringe();  // Close Main Valve & Open Refill Valve
    reverseMotor(reverseSpeed);    // Reverse in high speed (high step speed in micro-sec)
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
    ctrlValveStatesFillSyringe();  // Close Main Valve & Open Refill Valve
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
    ctrlValveStatesFlowSyringe();  // Close Refill Valve & Open Main Valve
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