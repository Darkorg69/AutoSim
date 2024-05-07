//DEBUG
constexpr bool debugEnabled = true;

////ENGINE
//Parameters
constexpr uint8_t engineStrokes = 4;    //[This value should either be 2 or 4]
constexpr uint8_t engineCylinders = 1;  //This value should be between 1 and your max PWM outputs
constexpr uint8_t engineFiringOrder[engineCylinders] = { 0 };
constexpr uint8_t degreesPerStroke = max(0, min(engineStrokes * 45, 180));
constexpr uint16_t degreesPerCycle = engineStrokes * degreesPerStroke;
constexpr uint16_t optimalFiringAngle = degreesPerCycle / engineCylinders;
//RPM
constexpr uint16_t engineStallRPM = 550;
constexpr uint16_t engineStartRPM = 650;
constexpr uint16_t engineLimitRPM = 6300;
//Coils
constexpr uint8_t minimumDwellTime = 3;                               // [milliseconds]
constexpr uint8_t ignitionEnd = 25;                                   // [° before TDC]
constexpr uint8_t ignitionEndAngle = degreesPerStroke - ignitionEnd;  // [° after BDC]
//Injectors
constexpr uint8_t injectionEnd = 50;                                        // [° before TDC]
constexpr uint8_t injectionEndAngle = degreesPerStroke - injectionEnd;      // [° after BDC]
constexpr uint8_t injectionStart = 90;                                      // [° before TDC]
constexpr uint8_t injectionStartAngle = degreesPerStroke - injectionStart;  // [° after BDC]

////PINS
constexpr uint8_t rpmAnalogInput{ A0 };                    //{ A0, A1, A2, A3, A4, A5 };
constexpr uint8_t ignitionPins[engineCylinders] = { 3 };   //{ 3, 5, 6 };
constexpr uint8_t injectionPins[engineCylinders] = { 9 };  //{ 9, 10, 11 };

//// ENGINE VARIABLES
static uint8_t currentCylinder = 0;
static bool engineIsRunning = false;
static uint16_t currentRPM = 0;
static double crankshaftAngle = 0.0;

static uint16_t crankshaftMinutes;  //1 ° = 60 minute
static uint16_t crankshaftSeconds;  //1 ° = 3600 second (60m*60s)



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
  for (auto &i : injectionPins) pinMode(i, OUTPUT);
}

uint8_t calculateDwellAngle() {
  return min(currentRPM * 0.006 * minimumDwellTime, injectionEndAngle);
}


void resetEngine() {
  crankshaftAngle = 0.0;

  for (uint8_t i = 0; i < engineCylinders; i++) {
    digitalWrite(ignitionPins[i], LOW);
    digitalWrite(injectionPins[i], LOW);
  }
}

void loop() {
  static uint32_t lastAccTime;

  crankshaftAngle = fmod(crankshaftAngle, degreesPerCycle);
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
        digitalWrite(injectionPins[i], LOW);
      }
      return;
    }

    currentCylinder = static_cast<uint8_t>(crankshaftAngle / optimalFiringAngle) % engineCylinders;

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

    uint8_t currentInjectionPin = injectionPins[engineFiringOrder[currentCylinder]];
    uint8_t currentCylinderInjection = digitalRead(currentInjectionPin);

    if (currentCylinderInjection == LOW && currentCylinderPistonPosition > injectionStartAngle && currentCylinderPistonPosition < injectionEndAngle) {
      digitalWrite(currentInjectionPin, HIGH);
    }

    if (currentCylinderInjection == HIGH && currentCylinderPistonPosition >= injectionEndAngle) {
      digitalWrite(currentInjectionPin, LOW);
    }
    
    int crankshaftAngleMinutesPerMinute = currentRPM * 360 * 60;
    int crankshaftAngleMinutesPerSecond = crankshaftAngleMinutesPerMinute / 60;
    int crankshaftAngleMinutesPerMillisecond = crankshaftAngleMinutesPerSecond / 1000; 
    int crankshaftAngleMinutesPerMicrosecond = crankshaftAngleMinutesPerMillisecond / 1000;

    int crankshaftAngleSecondsPerMinute = crankshaftAngleMinutesPerMinute * 60;
    int crankshaftAngleSecondsPerSecond = crankshaftAngleSecondsPerMinute / 60;
    int crankshaftAngleSecondsPerMillisecond = crankshaftAngleSecondsPerSecond / 1000;
    int crankshaftAngleSecondsPerMicrosecond = crankshaftAngleSecondsPerMillisecond / 1000;

    uint32_t currentRevolutionsPerSecond = currentRPM / 60;

    double currentRevolutionsPerMillisecond = currentRPM / 60 / 1000;
    double currentRevolutionsPerMicrosecond = currentRPM / 60 / 1000 / 1000;


    uint32_t crankshaftAngleMinutesPerMillisecond = currentRPM * 0.36;
    uint32_t crankshaftAngleSecondPerMillisecond = currentRPM * 21.6;

    double anglePerMicrosecond =  //0.0378

      uint32_t now = micros();
    crankshaftAngle += anglePerMicrosecond * (now - lastAccTime);
    lastAccTime = now;
  }
}