//// MACROS
/// PINS
// Analog Input
#define RPM_ANALOG_INPUT A0
// PWM Ignition Output
#define IGN_OUTPUT_0 3
#define IGN_OUTPUT_1 5
#define IGN_OUTPUT_2 6
// PWM Injection Output
#define INJ_OUTPUT_0 9
#define INJ_OUTPUT_1 10
#define INJ_OUTPUT_2 11

//// ENGINE
// Parameters
#define ENGINE_STROKES 4
#define ENGINE_CYLINDERS 1
#define DEGREES_PER_STROKE 180
#define DEGREES_PER_CYCLE (ENGINE_STROKES * DEGREES_PER_STROKE)
#define OPTIMAL_FIRING_ANGLE (DEGREES_PER_CYCLE / ENGINE_CYLINDERS)
// RPM
#define ENGINE_STALL_RPM 550
#define ENGINE_START_RPM 600
#define ENGINE_IDLE_RPM 650
#define ENGINE_LIMIT_RPM 6300
// Ignition and fuel injection timing [Degrees° before TDC]
#define INJ_START 90
#define INJ_END 45
#define IGNITION_ADVANCE 25

const bool debugEnabled = true; //change to false to disable serial monitoring

//// GLOBAL CONSTANTS (These should get manually updated based on macros)
// Unmodifiable array to store engine firing order
const uint8_t engineFiringOrder[ENGINE_CYLINDERS] = { 0 };
// Unmodifiable array to loop through ignition signal pins
const uint8_t ignitionSignalPins[ENGINE_CYLINDERS] = { IGN_OUTPUT_0 };
//const uint8_t ignitionSignalPins[ENGINE_CYLINDERS] = { IGN_OUTPUT_0, IGN_OUTPUT_1, IGN_OUTPUT_2 };
// Unmodifiable array to loop through injection signal pins
const uint8_t injectionSignalPins[ENGINE_CYLINDERS] = { INJ_OUTPUT_0 };
//const uint8_t injectionSignalPins[ENGINE_CYLINDERS] = { INJ_OUTPUT_0, INJ_OUTPUT_1, INJ_OUTPUT_2 };

//// PROGRAM VARIABLES
// Variable to store when each loop starts (Used to update crankshaft angle)
uint32_t loopStartTime = 0;

//// GLOBAL VARIABLES
// Variable to store the current RPM signal (pin A0)
uint16_t rpmSignal = 0;
// Variable to store the current mapped RPM signal (static current rpm)
uint16_t rpmSignalMapped = 0;

//// ENGINE VARIABLES
// Variable to store the current engine state
bool engineIsRunning = false;
//Variable to track if the engine should start
bool engineShouldStart = false;
//Variable to track if the engine should stall
bool engineShouldStall = false;
// Variable to store the current engine rpm
uint16_t currentRPM = 0;
// Variable to store the current crankshaft angle
double crankshaftAngle = 0.0;
//Variable to track which cylinder is at compression stroke
uint8_t currentCylinder = 0;
/// Array to track cylinders spark ignition
uint8_t cylinderIgnition[ENGINE_CYLINDERS] = { LOW };
/// Array to track cylinder fuel injection
uint8_t cylinderInjection[ENGINE_CYLINDERS] = { LOW };

void setup() {
  //Put your setup code here, to run once:
  Serial.begin(9600);

  //Initialize ignition pin modes
  pinMode(IGN_OUTPUT_0, OUTPUT);
  pinMode(IGN_OUTPUT_1, OUTPUT);
  pinMode(IGN_OUTPUT_2, OUTPUT);
  //Initialize injection pin modes
  pinMode(INJ_OUTPUT_0, OUTPUT);
  pinMode(INJ_OUTPUT_1, OUTPUT);
  pinMode(INJ_OUTPUT_2, OUTPUT);
}

//like `%` but for doubles
double doubleModulus(double value, double divisor) {
  uint32_t timeSpent = micros();
  while (value >= divisor) {
    value -= divisor;
  }
  return value;
  Serial.print("Time spent on doubleModulus: ");
  Serial.print(timeSpent);
  Serial.print("\n");
}


void startEngine() {
  engineIsRunning = true;
  crankshaftAngle = 0.0;
  currentRPM = rpmSignalMapped;
  loopStartTime = micros();
}

void stallEngine() {
  engineIsRunning = false;
  crankshaftAngle = 0.0;
  currentRPM = 0;
}

uint32_t lastIgnitionCoilSignal = 0;
uint32_t lastInjectorSignal = 0;

void updateSignals() {
  for (uint8_t i = 0; i < ENGINE_CYLINDERS; i++) {
    digitalWrite(ignitionSignalPins[i], cylinderIgnition[i]);
    digitalWrite(injectionSignalPins[i], cylinderInjection[i]);
  }
}

void loop() {
  loopStartTime = micros();

  uint16_t rpmSignalNew = analogRead(RPM_ANALOG_INPUT);

  if (rpmSignal != rpmSignalNew) {
    rpmSignal = rpmSignalNew;
    rpmSignalPrint(rpmSignal);

    rpmSignalMapped = map(rpmSignal, 0, 1023, 0, ENGINE_LIMIT_RPM);
    rpmSignalMappedPrint(rpmSignalMapped);

    engineShouldStart = !engineIsRunning && (rpmSignalMapped >= ENGINE_START_RPM);
    engineShouldStall = engineIsRunning && (rpmSignalMapped <= ENGINE_STALL_RPM);

    if (engineShouldStart) {
      startEngine();
    }

    if (engineShouldStall) {
      stallEngine();
    }
  }

  if (engineIsRunning) {
    currentRPM = rpmSignalMapped;

    if ()
    //Serial.print(currentRPM);
    //Serial.print("\n");
    currentCylinder = crankshaftAngle / OPTIMAL_FIRING_ANGLE;

    if (doubleModulus(crankshaftAngle, DEGREES_PER_STROKE) + IGNITION_ADVANCE >= DEGREES_PER_STROKE) {
      cylinderIgnition[engineFiringOrder[currentCylinder]] = HIGH;
    } else {
      for (unsigned char i = 0; i < ENGINE_CYLINDERS; i++) {
        cylinderIgnition[i] = LOW;
      }
    }

    if (doubleModulus(crankshaftAngle, DEGREES_PER_STROKE) + INJ_START >= DEGREES_PER_STROKE) {
      cylinderInjection[engineFiringOrder[currentCylinder]] = HIGH;
    }

    if (doubleModulus(crankshaftAngle, DEGREES_PER_STROKE) + INJ_END >= DEGREES_PER_STROKE) {
      cylinderInjection[engineFiringOrder[currentCylinder]] = LOW;
    }

    updateSignals();
    crankshaftAngle = doubleModulus(crankshaftAngle + (currentRPM * 0.000006 * (micros() - loopStartTime)), DEGREES_PER_CYCLE);
  }
}