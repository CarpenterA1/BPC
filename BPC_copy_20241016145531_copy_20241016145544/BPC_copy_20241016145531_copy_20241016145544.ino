const int buttonPin = 2;         // Pushbutton pin
const int TransistorGate = 13;   // Transistor gate pin
const int oscilliometricPin = 11;   // Place holder 11 currently
const int arraysize = 100;

int inflationState = LOW;        // Current state of the transistor
int buttonState;                 // Current reading from the input pin
int lastButtonState = LOW;       // Previous reading from the input pin

int maxValues[arraySize];        // Max Value array for Oscilliometric Signal
int minValues[arraySize];        // Min Value array for Oscilliometric Signal
int maxAmplitude = 0;            // Max Value from Oscilliometric
int minAmplitude = 2.7;          // Min Value from Oscilliometric
int Samples = 0;
int Oscillometric;

unsigned long lastDebounceTime = 0; // Last time the button state changed
unsigned long debounceDelay = 50;   // Debounce delay in milliseconds

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(TransistorGate, OUTPUT);
  pinMode(Oscilliometric, INPUT);   // Added in post
  // Set initial transistor state
  digitalWrite(TransistorGate, inflationState);
}

void loop() {
  // Read the state of the button into a local variable
  int reading = digitalRead(buttonPin);

  // Check if the voltage is above the threshold first
  int sensorValue = analogRead(A1);
  float voltage = sensorValue * (5.0 / 1023.0);
  if (voltage > 3.4) {
    // If voltage is above threshold, turn off the transistor
    inflationState = LOW;
    digitalWrite(TransistorGate, inflationState);
  } else {
    // If the switch changed, due to noise or pressing:
    if (reading != lastButtonState) {
      // Reset the debouncing timer
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      // Whatever the reading is, it's been there for longer than the debounce
      // delay, so take it as the actual current state

      // If the button state has changed:
      if (reading != buttonState) {
        buttonState = reading;

        // Only toggle the state if the new button state is HIGH
        if (buttonState == HIGH) {
          inflationState = !inflationState;
        }
      }
    }

    // Set the transistor gate based on the button state (if voltage is not high)
    digitalWrite(TransistorGate, inflationState);
  }

//Stolimentric and diastolic signal
//=====================================================
  Oscillometric = analogRead(oscilliometricPin);

if 

  if (Oscillometric > maxAmplitude) {
    maxAmplitude = Oscillometric;
  }
  if (Oscillometric < minAmplitude) {
    minAmplitude = Oscillometric;
  }

  if (Samples < arraySize) {
    maxAmplitude[Samples] = maxAmplitude;
    minAmplitude[Samples] = minAmplitude;
    Samples++;
  }

  if (Samples == arraySize) {
    for (int i = 0, i < arraySize, i++){
      maxAmplitude[Samples] - minAmplitude[Samples];
    }

  }

  int K_s = .5;
  int K_d = .8;

  float A_d = A_max * K_d;
  float A_s = A_max * K_s;

  Serial.print("Mean Diastolic Signal is: ")
  Serial.println(diastolicSignal);

  Serial.print("Mean Systolic Signal is: ")
  Serial.println(systolicSignal);
  
  Serial.print("Mean Arterial Signal is: ")
  Serial.println(mean_arterialSignal);
//=====================================================

  // Save the reading. Next time through the loop, it'll be the lastButtonState
  lastButtonState = reading;

  // Print the voltage to the Serial Monitor for debugging
  Serial.print("Voltage: ");
  Serial.println(voltage);
  Serial.print("intating?");
  Serial.println(inflationState);

}