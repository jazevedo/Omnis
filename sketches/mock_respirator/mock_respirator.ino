#include <timer.h>

void printVariables(float tidalVolume, float pressureSupport, float respiratoryRate, char* inspExp, float peep);

auto timer = timer_create_default();

#define INPUT_LEN 8
#define INSP_EXP_LEN 4
#define MAX_BUFFER 512
char buffer[MAX_BUFFER];

int bufferIndex = 0;

#define NUM_OUTS 5
#define VOL_IN 0
#define VOL_OUT 1
#define FLOW_IN 2
#define FLOW_OUT 3
#define PRESSURE 4

bool sendOutput(void *argument /* optional argument given to in/at/every */) {  

  float outputs[NUM_OUTS];
  for (int i = 0; i < NUM_OUTS; i++) {
    outputs[i] = 10 * (i + 1) + (1 / (float)random(1, 30));
  }
  
  String outputMessage = "";
  for (int i = 0; i < NUM_OUTS; i++) {
    outputMessage += String(outputs[i], 4);
    if (i != NUM_OUTS - 1)
      outputMessage += ",";  
  }
  Serial.println(outputMessage);
}

void setup() {
  Serial.begin(9600);
  timer.every(3000, sendOutput);
}


void processBuffer() {
  char *inputBuffers[5];
  
  int i = 0;
  char* ptr = strtok(buffer, ",");
  while (ptr != NULL && i < 5) {
    inputBuffers[i++] = ptr;
    ptr = strtok(NULL, ",");
  }

  float tidalVolume = String(inputBuffers[0]).toFloat();
  float pressureSupport = String(inputBuffers[1]).toFloat();
  float respiratoryRate = String(inputBuffers[2]).toFloat();
  char inspExp[INSP_EXP_LEN];
  strncpy(inspExp, inputBuffers[3], strlen(inputBuffers[3]) + 1);
  float peep = String(inputBuffers[4]).toFloat();

  //printVariables(tidalVolume, pressureSupport, respiratoryRate, inspExp, peep);
  
  bufferIndex = 0;
}

void loop() {
  timer.tick();
  if (Serial.available() > 0) {
    do {
      int b = Serial.read();
      if (b == ';') {
        buffer[bufferIndex] = '\0';
        processBuffer();
      }
      else {
        buffer[bufferIndex++] = (char)b;
      }
    }
    while (Serial.available() > 0);
  }
}

void printVariables(float tidalVolume, float pressureSupport, float respiratoryRate, char* inspExp, float peep) {
  char buffer2[MAX_BUFFER];
  sprintf(buffer2, "tidalVolume = %s", String(tidalVolume, 4).c_str());
  Serial.println(buffer2);
  sprintf(buffer2, "pressureSupport= %s", String(pressureSupport, 4).c_str());
  Serial.println(buffer2);
  sprintf(buffer2, "respiratoryRate= %s", String(respiratoryRate, 4).c_str());
  Serial.println(buffer2);
  sprintf(buffer2, "inspExp= %s", inspExp);
  Serial.println(buffer2);
  sprintf(buffer2, "peep= %s", String(peep, 4).c_str());
  Serial.println(buffer2);
  
}
