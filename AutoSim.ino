////ENGINE
//Parameters
constexpr uint8_t engineStrokes = 4;    //[This value should either be 2 or 4]
constexpr uint8_t engineCylinders = 4;  //This value should be between 1 and your max PWM outputs
constexpr uint8_t engineFiringOrder[engineCylinders] = { 0, 2, 1, 3 };
constexpr uint8_t degreesPerStroke = max(0, min(engineStrokes * 45, 180));
constexpr uint16_t degreesPerCycle = engineStrokes * degreesPerStroke;
constexpr uint16_t optimalFiringAngle = degreesPerCycle / engineCylinders;
//RPM
constexpr uint16_t engineStallRPM = 300;    //30
constexpr uint16_t engineStartRPM = 600;   //120
constexpr uint16_t engineLimitRPM = 6000;  //3000
//Coils
constexpr uint8_t minimumDwellTime = 3;                               // [milliseconds]
constexpr uint8_t ignitionEnd = 25;                                   // [° before TDC]
constexpr uint8_t ignitionEndAngle = degreesPerStroke - ignitionEnd;  // [° after BDC]
////PINS
constexpr uint8_t rpmAnalogInput{ A0 };  //{ A0, A1, A2, A3, A4, A5 };
constexpr uint8_t ignitionPins[engineCylinders] = { 3, 5, 6, 11 };

//// ENGINE VARIABLES
static uint8_t currentCylinder = 0;
static bool engineIsRunning = false;
static uint16_t currentRPM = 0;
static double crankshaftAngle = 0.0;

////MATH FUNC
//like `%` but for doubles
double fmodulus(double value, double divisor) {
  while (value >= divisor) {
    value -= divisor;
  }

  return value;
}

void setup() {
  Serial.begin(115200);
  for (auto &i : ignitionPins) pinMode(i, OUTPUT);
}

uint8_t calculateDwellAngle() {
  return min(currentRPM * 0.006 * minimumDwellTime, ignitionEndAngle);
}


void resetEngine() {
  crankshaftAngle = 0.0;

  for (uint8_t i = 0; i < engineCylinders; i++) {
    digitalWrite(ignitionPins[i], LOW);
  }
}

void loop() {
  static uint32_t lastAccTime;

  crankshaftAngle = fmod(crankshaftAngle, degreesPerCycle);
  Serial.println(crankshaftAngle, 0);
  currentRPM = map(analogRead(rpmAnalogInput), 0, 1023, 0, engineLimitRPM);

  if (!engineIsRunning) {
    if (currentRPM >= engineStartRPM) {
      engineIsRunning = true;
      lastAccTime = micros();
    }
  } else {
    if (currentRPM <= engineStallRPM) {
      crankshaftAngle = 0.0;
      engineIsRunning = false;
      for (uint8_t i = 0; i < engineCylinders; i++) {
        digitalWrite(ignitionPins[i], LOW);
      }
      return;
    }

    currentCylinder = static_cast<uint8_t>(crankshaftAngle / optimalFiringAngle) % engineCylinders;
    Serial.println(currentCylinder);
    // Current cylinder piston position with respect to BDC. When this is 180 we are at TDC
    uint8_t currentCylinderPistonPosition = static_cast<uint8_t>(crankshaftAngle) % degreesPerStroke;
    

    uint8_t currentIgnitionPin = ignitionPins[engineFiringOrder[currentCylinder]];
    uint8_t currentCylinderIgnition = digitalRead(currentIgnitionPin);

    if (currentCylinderIgnition == LOW && currentCylinderPistonPosition >= ignitionEndAngle - calculateDwellAngle() && currentCylinderPistonPosition <= ignitionEndAngle) {
      digitalWrite(currentIgnitionPin, HIGH);
    }

    if (currentCylinderIgnition == HIGH && currentCylinderPistonPosition >= ignitionEndAngle) {
      digitalWrite(currentIgnitionPin, LOW);
    }

    double anglePerMicrosecond = currentRPM * 0.000006;
    uint32_t now = micros();
    crankshaftAngle += anglePerMicrosecond * (now - lastAccTime);
    lastAccTime = now;
  }
}