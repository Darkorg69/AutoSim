constexpr bool debugEnabled = true;  //change to false to disable serial monitoring
//// ENGINE
// Parameters
constexpr uint8_t engineStrokes = 4;    //This value should either be 2 or 4.
constexpr uint8_t engineCylinders = 1;  //This value should be between 1 and 3 when using the Arduino Uno. Switch according to how many PWM pins you have.

constexpr uint8_t degreesPerStroke = engineStrokes == 2 ? 90 : engineStrokes == 4 ? 180
                                                                                  : 0;
constexpr uint8_t engineFiringOrder[engineCylinders] = { 0 };

constexpr uint16_t degreesPerCycle = engineStrokes * degreesPerStroke;
constexpr uint16_t optimalFiringAngle = degreesPerCycle / engineCylinders;
// RPM
constexpr uint16_t engineStallRPM = 10;
constexpr uint16_t engineStartRPM = 60;
constexpr uint16_t engineLimitRPM = 6300;
// Coils
constexpr uint8_t minimumDwellTime = 3;  // [milliseconds]
constexpr uint8_t ignitionAdvance = 25;  // [° before TDC]
constexpr double coilFireAngle = degreesPerStroke - ignitionAdvance;
// Injectors
constexpr uint8_t injectionStart = 90;  // [° before TDC]
constexpr uint8_t injectionStartAngle = degreesPerStroke - injectionStart;
constexpr uint8_t injectionEnd = 50;  // [° before TDC]
constexpr uint8_t injectionEndAngle = degreesPerStroke - injectionEnd;
//PINS
constexpr uint8_t rpmAnalogInput{ A0 };
constexpr uint8_t ignitionPins[engineCylinders] = { 3 };
//constexpr uint8_t ignitionPins[engineCylinders] = { 3, 5, 6 };
constexpr uint8_t injectionPins[engineCylinders] = { 9 };
//constexpr uint8_t injectionPins[engineCylinders] = { 9, 10, 11 };
// Variable to store the current RPM signal (from potentiometer)
static uint16_t rpmSignal = 0;
// Variable to store the current mapped RPM signal (static current rpm)
static uint16_t rpmSignalMapped = 0;

/// Array to track cylinders spark ignition
static uint8_t cylinderIgnition[engineCylinders] = { LOW };
/// Array to track cylinder fuel injection
static uint8_t cylinderInjection[engineCylinders] = { LOW };

//// ENGINE VARIABLES
static bool engineIsRunning = false;
//Variable to track which cylinder is at compression stroke
static uint8_t currentCylinder = 0;
static uint16_t currentRPM = 0;
static double crankshaftAngle = 0.0;

////MATH FUNC
//like `%` but for doubles
double doubleModulus(double value, double divisor) {
  while (value >= divisor) {
    value -= divisor;
  }
  return value;
}

void setup() {
  Serial.begin(9600);

  for (auto &i : ignitionPins) pinMode(i, OUTPUT);
  for (auto &i : injectionPins) pinMode(i, OUTPUT);
}

void updateIgnitionSignals() {
  for (uint8_t i = 0; i < engineCylinders; i++) {
    digitalWrite(ignitionPins[i], cylinderIgnition[i]);
  }
}

void updateInjectionSignals() {
  for (uint8_t i = 0; i < engineCylinders; i++) {
    digitalWrite(injectionPins[i], cylinderInjection[i]);
  }
}

double calculateDwellAngle() {
  return max(ignitionAdvance + 15, min(currentRPM * 0.006 * minimumDwellTime, 180.0));
}


void startEngine() {
  resetEngine();
  engineIsRunning = true;
  currentRPM = rpmSignalMapped;
}

void stallEngine() {
  resetEngine();
  engineIsRunning = false;
}

void resetEngine() {
  currentRPM = 0;
  currentCylinder = 0;
  crankshaftAngle = 0.0;

  for (uint8_t i = 0; i < engineCylinders; i++) {
    cylinderIgnition[i] = LOW;
    cylinderInjection[i] = LOW;
  }

  updateIgnitionSignals();
  updateInjectionSignals();
}

void loop() {
  static uint32_t lastAccTime;

  if (debugEnabled) {
    Serial.println(crankshaftAngle);
  }

  uint16_t rpmSignalNew = analogRead(rpmAnalogInput);

  if (rpmSignal != rpmSignalNew) {
    rpmSignal = rpmSignalNew;
    rpmSignalMapped = map(rpmSignal, 0, 1023, 0, engineLimitRPM);
    if (engineIsRunning) {
      if (rpmSignalMapped <= engineStallRPM) {
        stallEngine();
      } else {
        currentRPM = rpmSignalMapped;
      }
    } else {
      if (rpmSignalMapped >= engineStartRPM) {
        startEngine();
        lastAccTime = micros();
      }
    }
  }

  if (engineIsRunning) {
    currentCylinder = static_cast<uint8_t>(floor(crankshaftAngle / optimalFiringAngle)) % engineCylinders;
    // Current cylinder piston position with respect to BDC. When this is 180.0 we are at TDC
    double currentCylinderPistonPosition = fmod(crankshaftAngle, degreesPerStroke);
    uint8_t currentCylinderIgnition = cylinderIgnition[engineFiringOrder[currentCylinder]];

    if (currentCylinderIgnition == LOW && currentCylinderPistonPosition >= coilFireAngle - calculateDwellAngle() && currentCylinderPistonPosition <= coilFireAngle) {
      cylinderIgnition[engineFiringOrder[currentCylinder]] = HIGH;
      updateIgnitionSignals();
    }

    if (currentCylinderIgnition == HIGH && currentCylinderPistonPosition >= coilFireAngle) {
      cylinderIgnition[engineFiringOrder[currentCylinder]] = LOW;
      updateIgnitionSignals();
    }

    analogWriteFrequency()
    uint8_t currentCylinderInjection = cylinderInjection[engineFiringOrder[currentCylinder]];

    if (currentCylinderInjection == LOW && currentCylinderPistonPosition > injectionStartAngle && currentCylinderPistonPosition < injectionEndAngle) {
      cylinderInjection[engineFiringOrder[currentCylinder]] = HIGH;
      updateInjectionSignals();
    }

    if (currentCylinderInjection == HIGH && currentCylinderPistonPosition >= injectionEndAngle) {
      cylinderInjection[engineFiringOrder[currentCylinder]] = LOW;
      updateInjectionSignals();
    }

    uint32_t now = micros();
    crankshaftAngle = fmod(crankshaftAngle + (currentRPM * 0.000006 * (now - lastAccTime)), degreesPerCycle);
    lastAccTime = now;
  }
}