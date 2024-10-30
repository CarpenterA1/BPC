const int buttonPin = 2;         // Pushbutton pin
const int TransistorGate = 13;   // Transistor gate pin

int inflationState = LOW;        // Current state of the transistor
int buttonState;                 // Current reading from the input pin
int lastButtonState = LOW;       // Previous reading from the input pin

unsigned long lastDebounceTime = 0; // Last time the button state changed
unsigned long debounceDelay = 50;   // Debounce delay in milliseconds

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(TransistorGate, OUTPUT);
  // Set initial transistor state
  digitalWrite(TransistorGate, inflationState);
}

void loop() {
  // Read the state of the button into a local variable
  int reading = digitalRead(buttonPin);

  // Check if the voltage is above the threshold first
  int sensorValue = analogRead(A1);
  float voltage = sensorValue * (5.0 / 1023.0);
  if (voltage > 3) {
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

  // Save the reading. Next time through the loop, it'll be the lastButtonState
  lastButtonState = reading;

  // Print the voltage to the Serial Monitor for debugging
  Serial.print("Voltage: ");
  Serial.println(voltage);
  Serial.print("intating?");
  Serial.println(inflationState);

}