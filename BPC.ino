// Pin Definitions and Constants
const int signalPin = A1;         // Signal pin for oscillometric signal
const int pressurePin = A0;       // Pressure sensor pin
const int buttonPin = 2;          // Pushbutton pin
const int TransistorGate = 13;    // Transistor gate pin for cuff control

const float pressureThreshold = 3.45; // Start tracking when pressure voltage > 3V with .35v buffer
const float peakThreshold = 3.0;     // Voltage above which we start tracking a peak
const int sampleRate = 10;           // Sample rate in milliseconds
const int maxReadings = 100;         // Maximum number of readings
const unsigned long debounceDelay = 50; // Debounce delay in milliseconds

// Variables for Transistor Control
int inflationState = LOW;          // Current state of the transistor
int buttonState;                   // Current reading from the input pin
int lastButtonState = LOW;         // Previous reading from the input pin
unsigned long lastDebounceTime = 0;// Last time the button state changed

// Variables for Pulse Tracking
unsigned long currentTime;
bool tracking = false;               // Are we currently tracking pulses?
unsigned long trackingStartTime = 0; // Time when tracking started
int pulseCount = 0;

float pressureReadings[maxReadings];   // Array to store pressure readings in mmHg
float pulseAmplitudes[maxReadings];    // Array to store pulse amplitudes
unsigned long pulseTimes[maxReadings]; // Array to store pulse times

// Variables for Oscillometric Pulse
bool trackingPulse = false;
bool lookingForPeak = false;
bool lookingForValley = false;
float peakValue = 0.0;
float valleyValue = 5.0; // Initialize to max voltage to find the minimum
unsigned long lastPeakTime = 0;

// Variables for Signal Filtering
const int filterSize = 5;
float signalBuffer[filterSize];
int bufferIndex = 0;

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(TransistorGate, OUTPUT);
  // Set initial transistor state
  digitalWrite(TransistorGate, LOW);
}

void loop() {
  currentTime = millis();
  float pressureVoltage = analogRead(pressurePin) * (5.0 / 1023.0);

  // Convert pressure voltage to pressure in mmHg
  float pressure_mmHg = 50.0 * pressureVoltage; // 50 mmHg per volt

  // Transistor Control Logic
  if (!tracking) {
    int reading = digitalRead(buttonPin);

    // Debounce logic
    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != buttonState) {
        buttonState = reading;
        // Toggle inflation state if button is pressed
        if (buttonState == HIGH) {
          inflationState = !inflationState;
          digitalWrite(TransistorGate, inflationState);
        }
      }
    }

    lastButtonState = reading;

    // Stop inflation if pressure exceeds threshold and start tracking pulses
    if (pressureVoltage > pressureThreshold && inflationState == HIGH) { 
      inflationState = LOW;
      digitalWrite(TransistorGate, LOW);
      tracking = true;
      trackingStartTime = currentTime; // Record the time when tracking started
      pulseCount = 0; // Reset pulse count
      Serial.println("Started tracking pulses...");
    }
  }

  // Stop tracking when pressure voltage drops below a certain threshold (e.g., 1V)
  if (tracking && pressureVoltage < 1.0) {
    tracking = false;
    Serial.println("Stopped tracking pulses.");
    processResults();
  }

  if (tracking && (currentTime - trackingStartTime >= 3000)) {
    // Read oscillometric signal after 3-second delay
    float rawSignalVoltage = analogRead(signalPin) * (5.0 / 1023.0);
    float signalVoltage = getFilteredSignal(rawSignalVoltage);

    // Pulse detection logic
    if (!trackingPulse) {
      // Not currently tracking a pulse
      if (signalVoltage > peakThreshold) {
        // Signal voltage is above the peak threshold, start tracking pulse
        trackingPulse = true;
        lookingForPeak = true;
        peakValue = signalVoltage;
        lastPeakTime = currentTime;
      }
    } else {
      // Currently tracking a pulse
      if (lookingForPeak) {
        if (signalVoltage > peakValue) {
          // Still ascending towards the peak
          peakValue = signalVoltage;
          lastPeakTime = currentTime;
        } else {
          // Signal voltage decreased, peak has been found
          lookingForPeak = false;
          lookingForValley = true;
          valleyValue = signalVoltage;
        }
      } else if (lookingForValley) {
        if (signalVoltage < valleyValue) {
          // Still descending towards the valley
          valleyValue = signalVoltage;
        } else {
          // Signal voltage increased, valley has been found
          // Pulse is over
          lookingForValley = false;

          // Calculate pulse amplitude
          float pulseAmplitude = peakValue - valleyValue;

          // Only record if amplitude is greater than 0.4
          if (pulseAmplitude > 0.4) {
            // Record the amplitude and corresponding pressure
            if (pulseCount < maxReadings) {
              pulseAmplitudes[pulseCount] = pulseAmplitude;
              pressureReadings[pulseCount] = pressure_mmHg; // Store pressure in mmHg
              pulseTimes[pulseCount] = lastPeakTime;
              pulseCount++;
            }
          } else {
            // Ignore pulses with amplitude <= 0.4
            Serial.println("Pulse amplitude too low, ignoring.");
          }

          // Reset peak and valley values
          peakValue = 0.0;
          valleyValue = 5.0;
          trackingPulse = false;
        }
      }
    }
  }

  delay(sampleRate); // Delay for sampling rate
}

float getFilteredSignal(float newSignal) {
  signalBuffer[bufferIndex] = newSignal;
  bufferIndex = (bufferIndex + 1) % filterSize;
  float sum = 0;
  for (int i = 0; i < filterSize; i++) {
    sum += signalBuffer[i];
  }
  return sum / filterSize;
}

void processResults() {
  // Find the maximum pulse amplitude and its index
  float maxAmplitude = 0;
  int maxIndex = 0;
  for (int i = 0; i < pulseCount; i++) {
    if (pulseAmplitudes[i] > maxAmplitude) {
      maxAmplitude = pulseAmplitudes[i];
      maxIndex = i;
    }
  }

  // Check if we have enough data
  if (maxAmplitude == 0) {
    Serial.println("No valid pulses detected.");
    return;
  }

  // Adjusted thresholds based on empirical data
  float systolicThreshold = 0.55 * maxAmplitude;   
  float diastolicThreshold = 0.47 * maxAmplitude;  

  float systolicPressure = 0;
  float diastolicPressure = 0;

  // Find systolic pressure (before max amplitude)
  for (int i = maxIndex; i >= 0; i--) {
    if (pulseAmplitudes[i] <= systolicThreshold) {
      if (i < maxIndex) {
        // Linear interpolation between the two points
        float x1 = pulseAmplitudes[i];
        float x2 = pulseAmplitudes[i + 1];
        float y1 = pressureReadings[i];
        float y2 = pressureReadings[i + 1];
        systolicPressure = y1 + (systolicThreshold - x1) * (y2 - y1) / (x2 - x1);
      } else {
        systolicPressure = pressureReadings[i];
      }
      break;
    }
  }

  // Find diastolic pressure (after max amplitude)
  for (int i = maxIndex; i < pulseCount; i++) {
    if (pulseAmplitudes[i] <= diastolicThreshold) {
      if (i > maxIndex) {
        // Linear interpolation between the two points
        float x1 = pulseAmplitudes[i - 1];
        float x2 = pulseAmplitudes[i];
        float y1 = pressureReadings[i - 1];
        float y2 = pressureReadings[i];
        diastolicPressure = y1 + (diastolicThreshold - x1) * (y2 - y1) / (x2 - x1);
      } else {
        diastolicPressure = pressureReadings[i];
      }
      break;
    }
  }

  // If systolicPressure is still zero, use first detected pulse
  if (systolicPressure == 0 && maxIndex > 0) {
    systolicPressure = pressureReadings[0];
    Serial.println("Systolic pressure estimated from first detected pulse.");
  }

  // If diastolicPressure is still zero, use last detected pulse
  if (diastolicPressure == 0 && maxIndex < pulseCount - 1) {
    diastolicPressure = pressureReadings[pulseCount - 1];
    Serial.println("Diastolic pressure estimated from last detected pulse.");
  }

  // Calculate Mean Arterial Pressure (MAP)
  float mapPressure = pressureReadings[maxIndex];

  // Calculate BPM
  float bpm = 0;
  if (pulseCount >= 2) {
    float totalInterval = 0;
    for (int i = 1; i < pulseCount; i++) {
      totalInterval += (pulseTimes[i] - pulseTimes[i - 1]);
    }
    float averageInterval = totalInterval / (pulseCount - 1);
    bpm = 60000.0 / averageInterval;
  } else {
    Serial.println("Not enough pulses detected for BPM calculation.");
  }

  // Print results
  Serial.println("Measurement Results:");
  Serial.print("Systolic Pressure: ");
  Serial.print(systolicPressure);
  Serial.println(" mmHg");
  Serial.print("Diastolic Pressure: ");
  Serial.print(diastolicPressure);
  Serial.println(" mmHg");
  Serial.print("MAP Pressure: ");
  Serial.print(mapPressure);
  Serial.println(" mmHg");
  Serial.print("Calculated BPM: ");
  Serial.println(bpm);

  // Print pulse amplitude and pressure readings for analysis
  Serial.println("Pulse Amplitude vs. Pressure Readings:");
  for (int i = 0; i < pulseCount; i++) {
    Serial.print("Pressure: ");
    Serial.print(pressureReadings[i]);
    Serial.print(" mmHg, Amplitude: ");
    Serial.println(pulseAmplitudes[i]);
  }
}