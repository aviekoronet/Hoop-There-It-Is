
#define TIMERONE_PIN 9
#define FIFTYPER_DUTY 512
#define BUFFER_LEN 16
#define RECEIVER_COUNT 16
#define GOAL_LED 9
#define ERROR_LED 10
#define RECEIVER_PIN 11
#define TX_PIN 0
#define RX_PIN 1
#define BAUDRATE 9600
#define READPERIOD 1600
#define ZERO_THRESHOLD 8
#define TIMETHRESHOLD 1600

volatile unsigned char* storageBuffer;
volatile unsigned char* collectedData;

volatile unsigned char** lastDetects;
volatile unsigned char lastDetectLen;

volatile bool dataCollected;
volatile bool dataProcessed;
volatile bool goalDetected;
IntervalTimer readTimer1;
IntervalTimer readTimer2;

volatile unsigned char currentSelect;
volatile unsigned char inputCount;
volatile unsigned char detectCount;
volatile unsigned char currentMillis;


void setup() {
  // put your setup code here, to run once:
  currentSelect = 0;
  pinMode(GOAL_LED, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);

  pinMode(RECEIVER_PIN, INPUT);

  // Set up serial 1 to send and receive UART signals
  Serial1.setTX(TX_PIN);
  Serial1.setRX(RX_PIN);
  Serial1.begin(BAUDRATE);
  
  readTimer1.priority(128);
  readTimer1.begin(getRawInput, READPERIOD/4);
  
  
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

void processData(){
  char lastSelect = currentSelect;
  if(lastSelect == 0)
    lastSelect = RECEIVER_COUNT - 1;
  else
    lastSelect--;
  dataCollected = false;
  char pulseSum = 0; //number of pulses
  char pulseCount = 0; //total time high
  char consecutiveOnesCount = 0;
  char zeroCount = 0;
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
        pulseCount += consecutiveOnesCount;
        pulseSum++;
      }
      consecutiveOnesCount = 0;
    }
  }
  if(zeroCount >= ZERO_THRESHOLD)
  {
    volatile unsigned char temp[2] = {lastSelect, currentMillis};
    lastDetects[detectCount] = temp;
    detectCount++;
  //  pulseSum = 0;
  }
  //float avgFreq = 1.0f/(2.0f*COLLECTPERIOD*(float)(pulseSum)/(float)(pulseCount));
  dataProcessed = true;
    
}

void detectGoal(){
  char rangeStart = lastDetects[detectCount-1][0];
  char rangeEnd = rangeStart;
  char pulseEnd = lastDetects[detectCount-1][1];
  char pulseStart = pulseEnd - READPERIOD;
  for(signed char i = detectCount-2; i >= 0; --i)
  {
    char timeStamp = pulseStart - lastDetects[i][1];
    char currentID = lastDetects[i][0];
    if(currentID == rangeStart-1)
    {
      rangeStart = currentID;
      if (timeStamp < READPERIOD*1.5)
      {
        if( i > 1)
        {
          rangeEnd = currentID;
          pulseEnd = lastDetects[i][0];
          pulseStart = pulseEnd - READPERIOD;
        }
        else
        {
          detectCount -= 2;
          i = detectCount;
          delete lastDetects[0];
          delete lastDetects[1];
          while(i > 1)
          {
            lastDetects[i-2] = lastDetects[i];
            i--;
          }
        }
      }
      else
      {
        pulseStart = lastDetects[i][1] - READPERIOD;
      }
    }
    else
    {
      if(currentID >= rangeStart-1 && currentID <= rangeEnd+1)
      {
        if (timeStamp < READPERIOD*(RECEIVER_COUNT-(currentID-rangeEnd)))
        {
          pulseStart = lastDetects[i][1] - READPERIOD;
          if (currentID < rangeStart)
            rangeStart = currentID;
          if (currentID > rangeEnd)
            rangeEnd = currentID;
        }
        else 
        {
          if( i > 1)
          {
            rangeStart = currentID;
            rangeEnd = currentID;
            pulseEnd = lastDetects[i][0];
            pulseStart = pulseEnd - READPERIOD;
          }
          else
          {
            detectCount -= 2;
            i = detectCount;
            delete lastDetects[0];
            delete lastDetects[1];
            while(i > 1)
            {
              lastDetects[i-2] = lastDetects[i];
              i--;
            }
          }
        }
      }
    }
  }
  if(pulseEnd < currentMillis - TIMETHRESHOLD)
  {
    goalDetected = ((pulseEnd - pulseStart) > TIMETHRESHOLD);
  }
    
}

void loop() {
  // Main data processing control
  if(dataCollected && !dataProcessed)
  {
    processData();
  }
  if(detectCount > 1 && !goalDetected)
  {
    detectGoal();
  }
  if(goalDetected)
  {
    digitalWrite(GOAL_LED, HIGH);
    while(detectCount > 0)
    {
      delete lastDetects[detectCount];
      detectCount--;
      
    }
  }

}
