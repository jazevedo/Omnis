/* 
Outer Reef Technologies @2020
Project Omnis
Open Source Medical Ventilator in effort to combat COVID-19
Author: Jonathan Azevedo
www.outerreeftech.com
jazevedo@outerreeftech.com
April 5, 2020

Rev_C - Implemented timer based interrumpts
*/

#include <Wire.h>
#include <PID_v1.h>
#include <Adafruit_MotorShield.h>
#include <stdint.h>
#include "TeensyTimerTool.h"

using namespace TeensyTimerTool;

TwoWire *_i2c;

const int analogOutPin = 5;

byte sensorInAddress = 0x01;
byte sensorOutAddress = 0x20;
byte FLOW_AND_PRESSURE = 0x84;

#define MAX_FLOW_COUNTS 220000     //Max flow 200 Liters per Minute
#define MAX_PRESSURE_COUNTS 44000  //Max Presure is 40 cc of H2O
#define FULL_SCALE_FLOW 200 
#define FULL_SCALE_PRESSURE 40
#define MAX_DOUBLE 32767
#define PIP 35            //Peak Inspiratory Pressure 35 cc of H2O
#define CALIBRATE_FLOW 0x1c
#define CALIBRATE_PRESSURE 0x24
#define COMMAND_MODE 0
#define READING_MODE 5
#define PRESSURE_MODE 1
#define VOLUME_MODE 2
#define CPAP_MODE 3
#define CALIBRATION 4
#define MAX_BUFFER 54
#define NUM_OUTS 5
#define VOL_IN 0
#define VOL_OUT 1
#define FLOW_IN 2
#define FLOW_OUT 3
#define PRESSURE 4

//Timers
Timer t2(PIT); //
Timer t3(PIT);

//Inputs
double respiratoryRate = 0;  //Breaths per minute
double inspirationExpirationRatio = 0;  //1:2, 1:3, 1:1, 2:1, 3:1, 4:1
double pressureSupport = 0;  //Inspiration pressure in cc of H2O
double peep = 0;  //cc of H2O
double setFlowIn = 0;      //Liters per Minute
double setFlowOut = 0;     //Liters per Minute
double tidalVolumeIn = 500.0;
double tidalVolumeOut = 0.0;

double flow, pressure;

double pidFlow, pidPressure;
double flowMultiplier = FULL_SCALE_FLOW / (float)MAX_FLOW_COUNTS;
double pressureMultiplier = FULL_SCALE_PRESSURE / (float)MAX_PRESSURE_COUNTS;

double motorInPWM_Correction = 0;  //y = 0.6275x + 64.373  (Range of 65 - 225)

double motorInPWM = 0.0;
double motorOutPWM = 0.0;

double KpInP = 2;
double KiInP = 4.5;
double KdInP = .4;

double KpInF = 10;
double KiInF = 20;
double KdInF = .05;

double KpOut = .5;
double KiOut = .1;
double KdOut = .1;

//bool isValveInOpen;
bool isValveOutOpen;

bool isMotorInOn = false;
bool isMotorOutOn = false;

bool sensorIn = false;
bool sensorOut = false;

bool motorPressureInState = false;
bool motorPressureOutState = false;
bool motorFlowInState = false;
bool motorFlowOutState = false;

double measureFlow = 0;
double cummulativeVolumeIn = 0;
double cummulativeVolumeOut = 0;
double pidFlowIn = 0;
double pidFlowOut = 0;

char c;
char lastCommand = '@';

int currentMode = 0;

char inputBuffer[MAX_BUFFER];
int bufferIndex = 0;

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();  

// Select which 'port' M1, M2, M3 or M4. In this case, M1
//Adafruit_DCMotor *motorIn = AFMS.getMotor(1);
Adafruit_DCMotor *motorOut = AFMS.getMotor(3);
//Adafruit_DCMotor *valveIn = AFMS.getMotor(3);
Adafruit_DCMotor *valveOut = AFMS.getMotor(4);

//Specify the links and initial tuning parameters
PID motorControllerFlowIn(&pidFlow, &motorInPWM, &setFlowIn,KpInF,KiInF,KdInF, DIRECT); 
PID motorControllerFlowOut(&pidFlow, &motorOutPWM, &setFlowOut,KpOut,KiOut,KdOut, P_ON_M, REVERSE);
PID motorControllerPressureIn(&pidPressure, &motorInPWM, &pressureSupport,KpInP,KiInP,KdInP, P_ON_M, DIRECT); 
PID motorControllerPressureOut(&pidPressure, &motorOutPWM, &peep,KpOut,KiOut,KdOut, P_ON_M, REVERSE);

void processBuffer();
float convertRatio(char* s);
bool sendOutput(void *argument /* optional argument given to in/at/every */);
void standby();

void _calibrate(byte address, byte command) {
  _i2c->beginTransmission(address);
  _i2c->write(command);
  _i2c->write(0x00);
  delay(1000);
  _i2c->endTransmission();
}

void performCalibrations() {
  Serial.println(F("Calibrating..."));
  standby();    
  _calibrate(sensorInAddress, CALIBRATE_FLOW);
  _calibrate(sensorInAddress, CALIBRATE_PRESSURE);
  _calibrate(sensorOutAddress, CALIBRATE_FLOW);
  _calibrate(sensorOutAddress, CALIBRATE_PRESSURE);
  Serial.println(F("Calibration Sucessful"));
}

void sendUpdates() {
  if (currentMode != COMMAND_MODE) {
    sendOutput(NULL);
  }
}

void motorControl() {
  if (motorPressureInState == true) {
    motorControlPressureIn();
  }
  else if (motorPressureOutState == true) {
    motorControlPressureOut();
  }
  else if (motorFlowInState == true) {
    motorControlFlowIn();
  }
  else if (motorFlowOutState == true) {
    motorControlFlowOut();
  }
  else {
    //Serial.print("motor loop");
    ///do nothing
  }  
}

void resetMotorControllerPressureIn() {
  motorControllerPressureIn.SetOutputLimits(0.0, 1.0);  // Forces minimum up to 0.0
  motorControllerPressureIn.SetOutputLimits(-1.0, 0.0);  // Forces maximum down to 0.0
  motorControllerPressureIn.SetOutputLimits(120,250.0);
}  

void resetMotorControllerPressureOut() {
  motorControllerPressureOut.SetOutputLimits(0.0, 1.0);  // Forces minimum up to 0.0
  motorControllerPressureOut.SetOutputLimits(-1.0, 0.0);  // Forces maximum down to 0.0
  motorControllerPressureOut.SetOutputLimits(0.0,100.0);
}  

void resetMotorControllerFlowIn() {
  motorControllerFlowIn.SetOutputLimits(0.0, 1.0);  // Forces minimum up to 0.0
  motorControllerFlowIn.SetOutputLimits(-1.0, 0.0);  // Forces maximum down to 0.0
  motorControllerFlowIn.SetOutputLimits(120,250.0);
}  

void resetMotorControllerFlowOut() {
  motorControllerFlowOut.SetOutputLimits(0.0, 1.0);  // Forces minimum up to 0.0
  motorControllerFlowOut.SetOutputLimits(-1.0, 0.0);  // Forces maximum down to 0.0
  motorControllerFlowOut.SetOutputLimits(0.0,100.0);
}  

void motorControlPressureIn() {
  motorControllerPressureIn.Compute();
  analogWrite(analogOutPin, motorInPWM);
}

void motorControlPressureOut() {
  motorControllerPressureOut.Compute();
  motorOut->run(FORWARD);
  motorOut->setSpeed(motorInPWM);
}

void motorControlFlowIn() {
  motorControllerFlowIn.Compute();
  analogWrite(analogOutPin, motorInPWM);
}

void motorControlFlowOut() {
  motorControllerFlowOut.Compute();
  motorOut->run(FORWARD);
  motorOut->setSpeed(75);
}

void setup() {
  _i2c = &Wire;
  
  _i2c->begin();
  Serial.begin(115200);           // set up Serial library at 9600 bps

  Serial.println(F("Initializing Omnis Ventilator"));
  Serial.println();

  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(1000);  // OR with a different frequency, say 1KHz
 
  //timeLast = millis();
  
  motorControllerFlowIn.SetMode(MANUAL);
  motorControllerFlowIn.SetOutputLimits(120,250.0);
  motorControllerFlowIn.SetSampleTime(1);
  
  motorControllerFlowOut.SetMode(MANUAL);
  motorControllerFlowOut.SetOutputLimits(0.0,100.0);
  motorControllerFlowIn.SetSampleTime(1);
  
  motorControllerPressureIn.SetMode(MANUAL);
  motorControllerPressureIn.SetOutputLimits(120,250.0);
  motorControllerFlowIn.SetSampleTime(1);
  
  motorControllerPressureOut.SetMode(MANUAL);
  motorControllerPressureOut.SetOutputLimits(0.0,100.0);
  motorControllerFlowIn.SetSampleTime(1);

  standby();

  Serial.println();
  Serial.println(F("COMMANDS:"));
  Serial.println(F("c = Initialize Calibration"));
  Serial.println(F("mode tidalVolume, pressureSupport, respiratoryRate, inspirationExpirationRatio, peep"));
  Serial.println(F(" v500,15,10,1:2,3;  "));
  Serial.println();

  //delay(1000);

  //t1.beginPeriodic(checkSerial,   200'000); // 5Hz
  t2.beginPeriodic(motorControl,   500); // 2kHz
  t3.beginPeriodic(sendUpdates,   100'000); // 10Hz
  
}// End of Setup

int readWord(int expect) {
  int input = 0;
  if (expect == _i2c->available()) { // if 8 bytes were received
    input = lowByte(_i2c->read());  // receive high byte (overwrites previous reading)
    input = input << 8 ;    // shift high byte to be high 8 bits
    input |= lowByte(_i2c->read()); // receive low byte as lower 8 bits
    input = input << 8;    // shift high byte to be high 8 bits
    input |= lowByte(_i2c->read());  // receive high byte (overwrites previous reading)
    input = input << 8;    // shift high byte to be high 8 bits
    input |= lowByte(_i2c->read());  // receive high byte (overwrites previous reading)
  }
  else {
    Serial.print("readWord fail, avail = ");
    Serial.println(_i2c->available());
  }
  return input;
}

void clearBuffer() {
  if (_i2c->available() > 0) {
    for (int i = 0; i < _i2c->available(); i++) {
      _i2c->read();
    }
  }
}

void readSensor() {
  _i2c->beginTransmission(sensorInAddress);  //Begin transmission
  _i2c->write(FLOW_AND_PRESSURE); //Ask the particular registers for data
  _i2c->endTransmission(); //End transmission
  _i2c->requestFrom(sensorInAddress, 8);    // request flow and pressure data
  flow = readWord(8);
  pidFlow = flow * flowMultiplier; // range 0 - 200
  pressure = readWord(4);
  pidPressure = pressure * pressureMultiplier;// range 0 - 200
}

char getCommand() {
  if (Serial.available() > 0)
  {
    c = Serial.read();
  }
  else
  {
    c = lastCommand;
  }
  return c;
}

void standby() {
    motorOut->run(RELEASE);
    //valveIn->run(RELEASE);
    valveOut->run(RELEASE);
    sensorIn = false;
    motorPressureInState = false;
    motorPressureOutState = false;
    motorFlowInState = false;
    motorFlowOutState = false;
    delay(2);
    analogWrite(analogOutPin, 0);
    cummulativeVolumeIn = 0;
    cummulativeVolumeOut = 0;
}

/*
bool openValveIn(void *arguments) {
  valveIn->run(FORWARD);
  valveIn->run(RELEASE);
  isValveInOpen = true; 
}

bool closeValveIn(void *arguments) {
  valveIn->run(FORWARD);
  valveIn->setSpeed(255);
  isValveInOpen = false; 
}
*/

bool openValveOut(void *arguments) {
  valveOut->run(FORWARD);
  valveOut->setSpeed(255);
  isValveOutOpen = true; 
}

bool closeValveOut(void *arguments) {
  valveOut->run(FORWARD);
  valveOut->run(RELEASE);
  isValveOutOpen = false; 
}

void loop() {
if (currentMode == PRESSURE_MODE
    || currentMode == VOLUME_MODE
    || currentMode == CPAP_MODE) {
      run();
    }  
    if (currentMode == READING_MODE) {
      while (Serial.available() > 0) {
        inputBuffer[bufferIndex++] = (char)Serial.read();
        if (inputBuffer[bufferIndex - 1] == ';') {
          currentMode = COMMAND_MODE;
          inputBuffer[bufferIndex] = '\0';
          processBuffer();
  
          // lastCommand will contain p, v or a
          switch (lastCommand) {
            case 'p':
             currentMode = PRESSURE_MODE;
             break;
            case 'v':
              currentMode = VOLUME_MODE;
              break;
            case 'a':
              currentMode = CPAP_MODE;
              break;
            default:
              return;
          }
          lastCommand = '@';
          run();
        }
      }
    }
    else {        
      char command = getCommand();
      switch (command){
        case '@':
          break;
        case 'p': // pressure, read bytes
          lastCommand = 'p';
          currentMode = READING_MODE;
          bufferIndex = 0;
          break;
        case 'v': // volume, read bytes
          currentMode = READING_MODE;
          bufferIndex = 0;
          lastCommand = 'v';
          break;
        case 'a': // cpap, read bytes
          Serial.println("entering cpap mode");
          currentMode = READING_MODE;
          bufferIndex = 0;
          lastCommand = 'a';
          break;
        case 's':
          Serial.println(F("Standby"));
          standby();
          delay(1000);
          lastCommand = 's';
          currentMode = COMMAND_MODE;
          break;  
        case 'c':
          performCalibrations();
          lastCommand = '@';
          break;
        case 'r':
          run();
          lastCommand = 'r';
          break;
      }
    }  
}

void run() {  
//  unsigned long elapsedTime = millis() - timeLast;
  switch (currentMode) {
    case PRESSURE_MODE:
      pressureControl();
      break;
    case VOLUME_MODE:
      volumeControl();
      break;
    case CPAP_MODE:
      cpap();
      break;
  }
}

void pressureControl() {
  //Begin Inhale
  setFlowIn = (tidalVolumeIn *60)/((60/respiratoryRate)*(inspirationExpirationRatio)*1000);  //(tidalVolumeIn * 60 seconds) / (60 seconds / respiratoryRate) * (inspirationExpirationRatio) * conversion from lpm to mLpm
  setFlowOut = (tidalVolumeIn *60)/((60/respiratoryRate)*(1-inspirationExpirationRatio)*1000);  //(tidalVolumeIn * 60 seconds) / (60 seconds / respiratoryRate) * (1-inspirationExpirationRatio) * conversion from lpm to mLpm
  readSensor();
  //openValveIn(NULL);
  if (pidPressure < pressureSupport && pidPressure < PIP){
    motorControllerFlowIn.SetMode(AUTOMATIC);
    resetMotorControllerFlowIn();
    motorFlowInState = true;
    while (pidPressure < pressureSupport && pidPressure <= PIP){
      int start_in = micros();
      readSensor();
      if (pidFlow >= 1.0) {
        int stop_in = micros();
        cummulativeVolumeIn += (((pidFlow)/60.0) * ((stop_in - start_in)))/1000;
        cummulativeVolumeOut = 0;
      }
      else if (pidFlow <= -0.25) {
        int stop_in = micros();
        cummulativeVolumeOut += 1.2*((((pidFlow)/60.0) * ((stop_in - start_in)))/1000);
      }
      if (Serial.available() > 0) {
        motorFlowInState = false;
        motorControllerFlowIn.SetMode(MANUAL);
        break;
      }
    }
  //closeValveIn(NULL);
  motorFlowInState = false;
  motorControllerFlowIn.SetMode(MANUAL);
  analogWrite(analogOutPin, 0);
  }
  //End Inhale

  //Begin Exhale
  //openValveOut(NULL);
  delay(5);
  readSensor();
  if (pidPressure > (peep + 0.15)) {
    motorControllerFlowOut.SetMode(AUTOMATIC);
    resetMotorControllerFlowOut();
    motorFlowOutState = false;
    while (pidPressure > (peep + 0.15)) {
      int start_out = micros();
      readSensor();
      if (pidFlow <= 0.25) {
        int stop_out = micros();
        cummulativeVolumeOut += 1.2*((((pidFlow)/60.0) * ((stop_out - start_out)))/1000);
        cummulativeVolumeIn = 0;
      }
      else if (pidFlow <= -0.25) {
        int stop_out = micros();
        cummulativeVolumeIn = 0;
        //cummulativeVolumeIn += (((pidFlow)/60.0) * ((stop_out - start_out)))/1000;
      }
      if (Serial.available() > 0) {
        motorFlowOutState = false;
        motorControllerFlowOut.SetMode(MANUAL);
        break;
      }
    }
  //closeValveOut(NULL);
  motorFlowOutState = false;
  motorControllerFlowOut.SetMode(MANUAL);
  motorOut->run(FORWARD);
  motorOut->setSpeed(0); 
  } //End Exhale
}  //EndPressureControl

void volumeControl() {
  setFlowIn = (tidalVolumeIn *60)/((60/respiratoryRate)*(inspirationExpirationRatio)*1000);  //(tidalVolumeIn * 60 seconds) / (60 seconds / respiratoryRate) * (inspirationExpirationRatio) * conversion from lpm to mLpm
  setFlowOut = (tidalVolumeIn *60)/((60/respiratoryRate)*(1-inspirationExpirationRatio)*1000);  //(tidalVolumeIn * 60 seconds) / (60 seconds / respiratoryRate) * (1-inspirationExpirationRatio) * conversion from lpm to mLpm 
  readSensor();
  //openValveIn(NULL);
  if (cummulativeVolumeIn <= tidalVolumeIn && pidPressure < PIP){
    motorControllerFlowIn.SetMode(AUTOMATIC);
    resetMotorControllerFlowIn();
    motorFlowInState = true;
    while (cummulativeVolumeIn <= tidalVolumeIn && pidPressure < PIP){
      int start_in = micros();
      readSensor();
      if (pidFlow >= 1.0) {
        int stop_in = micros();
        cummulativeVolumeIn += (((pidFlow)/60.0) * ((stop_in - start_in)))/1000;
        cummulativeVolumeOut = 0;
      }
      else if (pidFlow <= -0.25) {
        int stop_in = micros();
        cummulativeVolumeOut += 1.2*((((pidFlow)/60.0) * ((stop_in - start_in)))/1000);
      }
      if (Serial.available() > 0) {
        motorFlowInState = false;
        motorControllerFlowIn.SetMode(MANUAL);
        break;
      }
    }
  //closeValveIn(NULL);
  cummulativeVolumeIn = 0;
  motorFlowInState = false;
  motorControllerFlowIn.SetMode(MANUAL);
  analogWrite(analogOutPin, 0);
  }
  //End Inhale

  //Begin Exhale
  //openValveOut(NULL);
  readSensor();
  if (pidPressure > (peep + 0.15)) {
    motorControllerFlowOut.SetMode(AUTOMATIC);
    resetMotorControllerFlowOut();
    motorFlowOutState = false;
    while (pidPressure > (peep + 0.15)) {
      int start_out = micros();
      readSensor();
      if (pidFlow <= 0.25) {
        int stop_out = micros();
        cummulativeVolumeOut += 1.2*((((pidFlow)/60.0) * ((stop_out - start_out)))/1000);
        cummulativeVolumeIn = 0;
      }
      else if (pidFlow <= -0.25) {
        int stop_out = micros();
        cummulativeVolumeIn = 0;
        //cummulativeVolumeIn += (((pidFlow)/60.0) * ((stop_out - start_out)))/1000;
      }
      if (Serial.available() > 0) {
        motorFlowOutState = false;
        motorControllerFlowOut.SetMode(MANUAL);
        break;
      }
    }
  //closeValveOut(NULL);
  motorFlowOutState = false;
  motorControllerFlowOut.SetMode(MANUAL);
  motorOut->run(FORWARD);
  motorOut->setSpeed(0); 
  }//End Exhale
}//End Volume Control



//Begin CPAP
void cpap() {
  readSensor();
  //openValveIn(NULL);
  if (pidPressure < PIP){
    motorControllerPressureIn.SetMode(AUTOMATIC);
    resetMotorControllerPressureIn();
    motorPressureInState = true;
    while (pidPressure < PIP){
      int start_in = micros();
      readSensor();
      if (pidFlow >= 1.0) {
        int stop_in = micros();
        cummulativeVolumeIn += (((pidFlow)/60.0) * ((stop_in - start_in)))/1000;
        cummulativeVolumeOut = 0;
      }
      else if (pidFlow <= -5.0) {
        int stop_in = micros();
        cummulativeVolumeOut += 1.2*((((pidFlow)/60.0) * ((stop_in - start_in)))/1000);
        cummulativeVolumeIn = 0;
      }
      if (Serial.available() > 0) {
        motorPressureInState = false;
        motorControllerPressureIn.SetMode(MANUAL);
        break;
      }
    }
  //closeValveIn(NULL);
  }
  else {
    analogWrite(analogOutPin, 0);
  }
}//End CPAP

void processBuffer() {
  char *tokens[5];
  int i = 0;
  char* ptr = strtok(inputBuffer, ",");
  while (ptr != NULL && i < 5) {
    tokens[i++] = ptr;
    ptr = strtok(NULL, ",");
  }

  // TODO: add error correct

  tidalVolumeIn = String(tokens[0]).toFloat();
  pressureSupport = String(tokens[1]).toFloat();
  respiratoryRate = String(tokens[2]).toFloat();
  inspirationExpirationRatio = convertRatio(tokens[3]);
  peep = String(tokens[4]).toFloat(); 
  bufferIndex = 0;
}

void printVariables(float tidalVolumeIn, float pressureSupport, float respiratoryRate, float inspirationExpirationRatio, float peep) {
  char buffer2[MAX_BUFFER];
  sprintf(buffer2, "tidalVolumeIn = %s", String(tidalVolumeIn, 4).c_str());
  Serial.println(buffer2);
  sprintf(buffer2, "pressureSupport= %s", String(pressureSupport, 4).c_str());
  Serial.println(buffer2);
  sprintf(buffer2, "respiratoryRate= %s", String(respiratoryRate, 4).c_str());
  Serial.println(buffer2);
  sprintf(buffer2, "inspExp= %s", String(inspirationExpirationRatio, 4).c_str());
  Serial.println(buffer2);
  sprintf(buffer2, "peep= %s", String(peep, 4).c_str());
  Serial.println(buffer2);  
}

float convertRatio(char* s) {
    if (strncmp("1:2", s, 3)==0) {
      return .3333333333;
    }
    if (strncmp("1:3", s, 3)==0) {
      return 0.25;
    }
    if (strncmp("1:1", s, 3)==0) {
      return 0.5;
    }
    if (strncmp("2:1", s, 3)==0) {
      return 0.6666666667;
    }
    if (strncmp("3:1", s, 3)==0) {
      return 0.75;
    }
    if (strncmp("4:1", s, 3)==0) {
      return 0.8;
    }
    return 0.0;
}

bool sendOutput(void *argument /* optional argument given to in/at/every */) {  
  float outputs[NUM_OUTS];
  outputs[VOL_IN] = cummulativeVolumeIn;
  outputs[VOL_OUT] = cummulativeVolumeOut;
  if (pidFlow > 1.25) {
    outputs[FLOW_IN] = pidFlow;  //mL per Minute X 1000
    outputs[FLOW_OUT] = 0;  //mL per Minute X 1000
  }
  else if (pidFlow < -0.5) {
    outputs[FLOW_IN] = 0;  //mL per Minute X 1000
    outputs[FLOW_OUT] = pidFlow;  //mL per Minute X 1000
  }
  else {
    outputs[FLOW_IN] = 0;  //mL per Minute X 1000
    outputs[FLOW_OUT] = 0;  //mL per Minute X 1000
  }
  outputs[PRESSURE] = pidPressure; 
  
  String outputMessage = "";
  for (int i = 0; i < NUM_OUTS; i++) {
    outputMessage += String(outputs[i], 4);
    if (i != NUM_OUTS - 1)
      outputMessage += ",";  
  }
  Serial.println(outputMessage);
}
