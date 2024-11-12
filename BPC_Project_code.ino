// Pin Definitions and Constants (unchanged from your original code)
const int signalPin = A1;
const int buttonPin = 2;
const int TransistorGate = 13;
const int pressurePin = A0;

const float pulseThreshold = 2.7;
const int sampleRate = 10;
const float pressureThreshold = 3;
const int maxPulses = 100;
const unsigned long debounceDelay = 50;
const unsigned long BPMCalculationDelay = 28000;

// Variables for Pulse Detection and Transistor Control (unchanged)
unsigned long currentTime;
bool trackingPulse = false;
unsigned long pulseTimes[maxPulses];
int pulseCount = 0;
int inflationState = LOW;
int buttonState;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
bool startBPMCalculation = false;
unsigned long BPMStartTime = 0;

// Arrays for Pressure Values and Timestamps
float pressureVoltages[maxPulses];
int pressureCount = 0;

// Arrays to Store Systolic, Diastolic, and MAP Values Over Time
float systolicValues[maxPulses];
float diastolicValues[maxPulses];
float mapValues[maxPulses];
unsigned long timestamps[maxPulses];
int valueCount = 0; // Counter for the stored values

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(TransistorGate, OUTPUT);
  digitalWrite(TransistorGate, inflationState);
}

void loop() {
  currentTime = millis();
  int signalValue = analogRead(signalPin);
  float pulseVoltage = signalValue * (5.0 / 1023.0);

  // Pulse detection logic (unchanged)
  if (pulseVoltage > pulseThreshold && !trackingPulse) {
    trackingPulse = true;
    if (pulseCount < maxPulses && startBPMCalculation) {
      pulseTimes[pulseCount] = currentTime;
      pulseCount++;
      if (pressureCount < maxPulses) {
        pressureVoltages[pressureCount] = pulseVoltage;
        pressureCount++;
      }
    }
  }
  if (pulseVoltage < pulseThreshold && trackingPulse) {
    trackingPulse = false;
  }

  int sensorValue = analogRead(pressurePin);
  float pressureVoltage = sensorValue * (5.0 / 1023.0);

  if (pressureVoltage > pressureThreshold && !startBPMCalculation) {
    startBPMCalculation = true;
    BPMStartTime = currentTime;
  }

  if (startBPMCalculation && (currentTime - BPMStartTime >= BPMCalculationDelay)) {
    if (pulseCount >= 2) {
      float totalInterval = 0;
      for (int i = 1; i < pulseCount; i++) {
        totalInterval += (pulseTimes[i] - pulseTimes[i - 1]);
      }
      float averageInterval = totalInterval / (pulseCount - 1);
      float bpm = 60000.0 / averageInterval;
      Serial.print("Calculated BPM: ");
      Serial.println(bpm);

      float systolicPressure = pressureVoltages[0];
      float diastolicPressure = pressureVoltages[0];
      for (int i = 1; i < pressureCount; i++) {
        if (pressureVoltages[i] > systolicPressure) {
          systolicPressure = pressureVoltages[i];
        }
        if (pressureVoltages[i] < diastolicPressure) {
          diastolicPressure = pressureVoltages[i];
        }
      }

      // Store Systolic & Diastolic values along with timestamp
      if (valueCount < maxPulses) {
        systolicValues[valueCount] = systolicPressure;
        diastolicValues[valueCount] = diastolicPressure;
        timestamps[valueCount] = currentTime;
        valueCount++;
      }

      // Print Systolic, Diastolic, and MAP values
      Serial.print("Systolic Pressure: ");
      Serial.println(systolicPressure);
      Serial.print("Diastolic Pressure: ");
      Serial.println(diastolicPressure);

      // Reset for the next cycle
      startBPMCalculation = false;
      pulseCount = 0;
      pressureCount = 0;
    } else {
      Serial.println("Not enough pulses detected for BPM calculation.");
      startBPMCalculation = false;
      pulseCount = 0;
      pressureCount = 0;
    }
  }

  // Button and Transistor Control Logic (unchanged)

  delay(sampleRate); // Delay for sampling rate
}

// // Function to Print All Recorded Values for Debugging
// void printRecordedValues() {
//   Serial.println("Recorded Pressure Values Over Time:");
//   for (int i = 0; i < valueCount; i++) {
//     Serial.print("Time: ");
//     Serial.print(timestamps[i]);
//     Serial.print(" | Systolic: ");
//     Serial.print(systolicValues[i]);
//     Serial.print(" | Diastolic: ");
//     Serial.print(diastolicValues[i]);
//     Serial.print(" | MAP: ");
//     Serial.println(mapValues[i]);
//   }
// }
