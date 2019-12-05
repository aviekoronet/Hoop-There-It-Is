#include <FrequencyTimer2.h>

#include <TimerThree.h>

#include <TimerOne.h>
#define EMITTERPERIOD1 1000L
#define EMITTERPERIOD2 200L
#define TIMERONE_PIN 9
#define FIFTYPER_DUTY 512
#define BUFFER_LEN 16
#define RECEIVER_COUNT 16

volatile unsigned char* storageBuffer;
volatile unsigned char* collectedData;

volatile unsigned char** lastDetects;
volatile unsigned char lastDetectLen;

volatile bool dataCollected;
volatile bool dataProcessed;
IntervalTimer emitterTimer1;
IntervalTimer emitterTimer2;
IntervalTimer readTimer1;
IntervalTimer readTimer2;

volatile unsigned char currentSelect;
volatile unsigned char inputCount;


void setup() {
  // put your setup code here, to run once:
  currentSelect = 0;
  pinMode(EMITTER1_PIN, OUTPUT);
  pinMode(EMITTER2_PIN, OUTPUT);
  pinMode(SELECT_PIN0 OUTPUT);
  pinMode(SELECT_PIN1, OUTPUT);
  pinMode(SELECT_PIN2, OUTPUT);
  pinMode(SELECT_PIN3, OUTPUT);
  pinMode(GOAL_LED, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);

  pinMode(RECEIVER_PIN, INPUT);

  // Set up serial 1 to send and receive UART signals
  Serial1.setTX(TX_PIN);
  Serial1.setRX(RX_PIN);
  Serial1.begin(BAUDRATE);
  
  Timer1.initialize(EMITTERPERIOD2);
  Timer1.pwm(TIMERONE_PIN,FIFTYPER_DUTY);
  Timer1.start();
  pinMode(FREQUENCYTIMER2_PIN, OUTPUT);
  FrequencyTimer2::setPeriod(EMITTERPERIOD1);
  FrequencyTimer2::enable();

  emitterTimer1.begin(toggleEmitter1, EMITTERPERIOD1);
  emitterTimer1.priority(100);
  emitterTimer2.begin(toggleEmitter2, EMITTERPERIOD2);
  emitterTimer2.priority(90);
  
  readTimer1.priority(128);
  readTimer1.begin(getRawInput, EMITTERPERIOD2/4);
  
  
  collectedData = new unsigned char[BUFFER_LEN];
  storageBuffer = new unsigned char[BUFFER_LEN];
  dataCollected = false;
  dataProcessed = true;
}

void getRawInput(){
  
  if(inputCount > 0)
  {
    if(inputCount <= BUFFER_LEN)
    {
      storageBuffer[inputCount-1] = digitalRead(RECEIVER_PIN);
    }
    else if(dataProcessed)
    {
      inputCount = 0;
      currentSelect++;
      currentSelect %= RECEIVER_COUNT;
      volatile unsigned char* tempPointer = collectedData;
      collectedData = storageBuffer;
      storageBuffer = tempPointer;
      dataCollected = true;
      dataProcessed = false;
    }
  }
  currentSelect++;
}
void getRawInput2(){
  
}

void processData(){
  char lastSelect = currentSelect;
  if(lastSelect == 0)
    lastSelect == RECEIVER_COUNT - 1;
  else
    lastSelect--;
  dataCollected = false;
  char pulseSum = 0;
  char pulseCount = 0;
  char consecutiveOnesCount = 0;
  char zeroCount = 0;
  bool freqMatch = false;
  for(char i = 0; i < BUFFER_LEN && zeroCount < ZERO_THRESHOLD; ++i)
  {
    if(collectedData[i] == HIGH)
    {
      consecutiveOnesCount++;
    }
    else
    {
      zeroCount++;
      if(consecutiveOnesCount > 0)
      {
        pulseCount++;
        pulseSum++;
      }
      consecutiveOnesCount = 0;
    }
  }
  if(zeroCount >= ZERO_THRESHOLD)
  {
    lastDetects[detectCount] = {lastSelect, currentMillis};
    detectCount++;
  //  pulseSum = 0;
  }
  //float avgFreq = 1.0f/(2.0f*COLLECTPERIOD*(float)(pulseSum)/(float)(pulseCount));
  dataProcessed = true;
    
}

void loop() {
  // put your main code here, to run repeatedly:
  if(dataCollected && !dataProcessed)
  {
    processData();
  }
  
  if(goalDetected)
  {
    digitalWrite(GOAL_LED, HIGH);
    while(detectCount > 0)
    {
      delete lastCollected[detectCount];
      detectCount--;
      
    }
  }

}
