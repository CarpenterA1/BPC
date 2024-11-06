// Pin Definitions
const int signalPin = A1;            // Analog pin for pulse signal
const int buttonPin = 2;             // Pushbutton pin
const int TransistorGate = 13;       // Transistor gate pin
const int pressurePin = A0;

// Constants
const float pulseThreshold = 2.7;        // Voltage threshold for pulse detection
const int sampleRate = 10;               // Time between samples in milliseconds
const float pressureThreshold = 3;       // Voltage threshold for transistor control
const int maxPulses = 100;               // Maximum number of pulses to store
const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds
const unsigned long BPMCalculationDelay = 28000; // 28 seconds in milliseconds

// Variables for Pulse Detection
unsigned long currentTime;
bool trackingPulse = false;          // Flag to indicate pulse tracking
unsigned long pulseTimes[maxPulses]; // Array to store pulse timestamps
int pulseCount = 0;                  // Number of pulses detected

// Variables for Transistor and Button Control
int inflationState = LOW;            // Current state of the transistor
int buttonState;                     // Current reading from the input pin
int lastButtonState = LOW;           // Previous reading from the input pin
unsigned long lastDebounceTime = 0;  // Last time the button state changed

// Variables for BPM Timing
bool startBPMCalculation = false;    // Flag to start BPM calculation timer
unsigned long BPMStartTime = 0;      // Start time for BPM calculation period

// Array to store pressure voltage values during pulse events
float pressureVoltages[maxPulses];    // Array to store pressure voltage values
int pressureCount = 0;                // Counter for stored pressure voltages

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(TransistorGate, OUTPUT);
  digitalWrite(TransistorGate, inflationState); // Set initial transistor state
}

void loop() {
  currentTime = millis();

  // Read the pulse signal value and convert it to voltage
  int signalValue = analogRead(signalPin);
  float pulseVoltage = signalValue * (5.0 / 1023.0);

  // Pulse detection logic
  if (pulseVoltage > pulseThreshold && !trackingPulse) {
    trackingPulse = true; // Start tracking the pulse

    // Record the pulse timestamp if space is available and if BPM calculation has started
    if (pulseCount < maxPulses && startBPMCalculation) {
      pulseTimes[pulseCount] = currentTime;
      pulseCount++;
      
      // Store the pressure voltage when pulse occurs after threshold is exceeded
      if (pressureCount < maxPulses) {
        pressureVoltages[pressureCount] = pulseVoltage; // Store the pressure voltage
        pressureCount++;
      }
    }
  }
  if (pulseVoltage < pulseThreshold && trackingPulse) {
    trackingPulse = false; // Stop tracking the pulse
  }

  // Read voltage on A1 for transistor control and BPM timer start
  int sensorValue = analogRead(pressurePin);
  float pressureVoltage = sensorValue * (5.0 / 1023.0);

  if (pressureVoltage > pressureThreshold && !startBPMCalculation) {
    // Start the 28-second timer for BPM calculation
    startBPMCalculation = true;
    BPMStartTime = currentTime;
  }

  // Check if 28 seconds have passed since the threshold was exceeded
  if (startBPMCalculation && (currentTime - BPMStartTime >= BPMCalculationDelay)) {
    // Calculate BPM from the collected pulse data
    if (pulseCount >= 2) {
      float totalInterval = 0;

      // Calculate intervals between consecutive pulses
      for (int i = 1; i < pulseCount; i++) {
        totalInterval += (pulseTimes[i] - pulseTimes[i - 1]);
      }

      // Calculate average interval and BPM
      float averageInterval = totalInterval / (pulseCount - 1);
      float bpm = 60000.0 / averageInterval;

      // Print BPM
      Serial.print("Calculated BPM: ");
      Serial.println(bpm);
    } else {
      Serial.println("Not enough pulses detected for BPM calculation.");
    }

    // Reset BPM calculation variables
    startBPMCalculation = false;
    pulseCount = 0;
  }

  // Button and Transistor Control Logic
  int reading = digitalRead(buttonPin);

  if (pressureVoltage > pressureThreshold) {
    // If voltage on A1 is above threshold, turn off the transistor
    inflationState = LOW;
    digitalWrite(TransistorGate, inflationState);
  } else {
    // Debounce button logic
    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != buttonState) {
        buttonState = reading;

        // Toggle inflation state on button press
        if (buttonState == HIGH) {
          inflationState = !inflationState;
        }
      }
    }

    // Set the transistor gate based on inflation state
    digitalWrite(TransistorGate, inflationState);
  }

  // Update lastButtonState
  lastButtonState = reading;

  // Print debugging information
  Serial.print("Pulse Voltage: ");
  Serial.print(pulseVoltage);
  Serial.print(" | Pressure Voltage: ");
  Serial.print(pressureVoltage);
  Serial.print(" | Inflation State: ");
  Serial.println(inflationState);

  // Print stored pressure voltages for debugging
  Serial.print("Stored Pressure Voltages: ");
  for (int i = 0; i < pressureCount; i++) {
    Serial.print(pressureVoltages[i]);
    Serial.print(" ");
  }
  Serial.println();

  delay(sampleRate); // Delay for sampling rate
}
