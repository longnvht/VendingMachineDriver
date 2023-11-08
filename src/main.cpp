#include <Arduino.h>

#include <TimerOne.h> 
//Config I/O
//595 connect Pin



// DS pin 14
const int DS =  4;
// SH_CP pin 11
const int SHCP =  5;
// ST_CP pin 12
const int STCP =  6;
//OE Pin 13
const int OE =  7;

//165 connect Pin
// LD pin 1
int latchPin = 8;
// Q7 pin 7
int dataPin = 9;
// CE pin 15
int enablePin = 11;
// CP pin 2
int clockPin = 12;

const int dropss = 3;
const int emFd =2;
const int enDrop = A2;
const int enA = A5;
const int enB = A4;
const int enC = A3;


byte current_state =HIGH, last_current_state=HIGH;
byte drop_state = LOW, last_drop_state= LOW;
int spiralSWCounter = 0;

int ejectStep=5;
int monitorStep = 5;
int frontDoorStep =5;
int spiralMotorStep = 5;
int topDoorStep =5;
int slotNumber;
int num1, num2, check_serial=0, check_tool=0;
String inString = "";
bool dataIn[16];
bool lastDataIn[16];
bool dataOut[24];
bool detectPosition = false;
int timerDetectPosition =0;
//define value set


int timerFrontDoor =0;
int delayOpenTime = 50;
int timerDetectTool=0;
int spiralTimer = 0;

int topDoorOpr = 0;
bool topDoorState = 0;
int frontDoorOpr = 0;
bool frontDoorState = 0;
int fbID =0;

void dropMonitor() ;
void read_serial();
void stopSpiral();
void shiftOutData(bool data[], int length);
void shiftInData();
void setMotorOuputStatus(int slot);
void rotateSpiral(int slot);
void frontDoorProcess();
void topDoorProcess();
void getToolProcess();
void emFrontDoor();
void spiralMotorProcess();

void openFrontDoor();
void closeFrontDoor();

void openTopDoor();
void closeTopDoor();

void frontDoorRev();
void frontDoorFwd();
void frontDoorStop();

void topDoorRev();
void topDoorFwd();
void topDoorStop();


void timerTick();
void showArray(bool array[], int length);
void copyArray(bool dest[], bool src[], int length);
bool compareArrays(bool arr1[], bool arr2[], int length);
void printData(bool data[], int length) ;



void setup() 
{
  Serial.begin(9600); // Start the Serial to show results in Serial Monitor
  while (!Serial) 
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  pinMode(SHCP, OUTPUT);// Latch PIN
  digitalWrite(SHCP, LOW);
  pinMode(STCP, OUTPUT);// Clock PIN
  digitalWrite(STCP, LOW);
  pinMode(DS, OUTPUT); //Data PIN
  digitalWrite(DS, LOW);
  pinMode(OE, OUTPUT);
  digitalWrite(OE, LOW);
  
  // Setup 74HC165 connections
  pinMode(dataPin, INPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(enablePin, OUTPUT);

  pinMode(dropss, INPUT);
  pinMode(enDrop, OUTPUT);
  digitalWrite(enDrop, HIGH);
  pinMode (enA, OUTPUT);
  pinMode (enB, OUTPUT);
  pinMode (enC, OUTPUT);
  pinMode (emFd, INPUT);

  attachInterrupt(0, emFrontDoor, RISING);

//    set timer interrupt
  Timer1.initialize(200000); // khởi tạo timer 1 đến 0.5 giây
  Timer1.attachInterrupt(timerTick); // khai báo ngắt timer 1

  stopSpiral();
  closeFrontDoor();
  closeTopDoor();

}
  
void loop() 
{
  while (Serial.available() > 0)
  {
    read_serial();
  }

  if (frontDoorStep < 2 || monitorStep < 2 || topDoorStep < 2 || spiralMotorStep < 2)
  {
    shiftInData();
  }

  dropMonitor();

  getToolProcess();

  topDoorProcess();
  
  frontDoorProcess();

  spiralMotorProcess();
}


bool compareArrays(bool arr1[], bool arr2[], int length) {
    for (int i = 0; i < length; i++) {
        if (arr1[i] != arr2[i]) {
            return false; // Có ít nhất một phần tử khác nhau
        }
    }
    return true; // Tất cả các phần tử giống nhau
}

void copyArray(bool dest[], bool src[], int length) {
    for (int i = 0; i < length; i++) {
        dest[i] = src[i];
    }
}

void showArray(bool array[], int length)
{
  for(int i =0; i < length; i++)
  {
    Serial.print(array[i]);
    Serial.print(' ');
  }
  Serial.println();
}

void shiftInData()
{
  digitalWrite(enablePin, HIGH);
  digitalWrite(latchPin, LOW);
  delayMicroseconds(5);
  digitalWrite(latchPin, HIGH);
  digitalWrite(enablePin, LOW);

  // Step 2: Shift
   for(int i = 0; i < 16; i++)
  {
      long bitVal = digitalRead(dataPin);
      //bytesVal |= (bitVal >>  i);
      dataIn[i]= bitVal & 1;

      digitalWrite(clockPin, HIGH);
      delayMicroseconds(5);
      digitalWrite(clockPin, LOW);
  }

}

  
void read_serial()
{
  int inChar = Serial.read();
  if(inChar == ',')
  {
    check_serial=1;
    num1=inString.toInt();
    inString = "";
  }
  else if(inChar == '\n')
  {
    if(check_serial==1)
    {
      num2=inString.toInt();
      check_serial=0;
      inString = "";
    }
    else 
    {
      num1 = inString.toInt();
      num2 = 0;
      check_serial=0;
      inString = "";
    }

    if(num1 == 100) Serial.println("120");
    if(num1 == 105) stopSpiral();

    if(num1 == 300) 
    {
      Serial.println("Open Front Door");
      openFrontDoor();
    }
    if(num1 == 301)
    {
      Serial.println("Close Front Door");
      closeFrontDoor();
    } 

    if(num1 == 400)
    {
      Serial.println("Open Top Door");
      openTopDoor();
    } 

    if(num1 == 401)
    {
      Serial.println("Close Top Door");
      closeTopDoor();
    } 
    if(num1 == 101)
    {
      if(num2>0 && num2<=60)
      {
        slotNumber=num2;
        ejectStep=0;
      }
    }
  }
  else inString += (char)inChar;
}


void rotateSpiral(int slot)
{
  setMotorOuputStatus(slot);
  shiftOutData(dataOut, 24);
}


void frontDoorProcess()
{
  if (frontDoorOpr ==1)
  {
    
    switch (frontDoorStep)
    {
    case 0:
      frontDoorRev();
      frontDoorStep ++;
      break;
    case 1:
      if(dataIn[13])
      {
        frontDoorStop();
        frontDoorStep ++;
        timerFrontDoor = 0;
        frontDoorState =1;
        frontDoorOpr =0;
      }

      break;
    
    }
  }

  if (frontDoorOpr ==2)
  {
    switch (frontDoorStep)
    {
    case 0:
      frontDoorFwd();
      frontDoorStep ++;
      break;
    case 1:
      if(dataIn[9])
      {
        frontDoorStop();
        frontDoorStep = 0;
        timerFrontDoor = 0;
        frontDoorOpr =0;
      }
      if (dataIn[12])
      {
        frontDoorStop();
        frontDoorState = 0;
        frontDoorOpr = 0;
        frontDoorStep ++;
      }
      break;
    }
  }
  
}

void topDoorProcess()
{
  if (topDoorOpr ==1)
  {
    
    switch (topDoorStep)
    {
    case 0:
      topDoorRev();
      topDoorStep ++;
      break;
    case 1:
      if(dataIn[15])
      {
        topDoorStop();
        topDoorStep ++;
        topDoorState =1;
        topDoorOpr =0;
      }

      break;
    }
  }

  if (topDoorOpr ==2)
  {
    switch (topDoorStep)
    {
    case 0:
      topDoorFwd();
      topDoorStep ++;
      break;
    case 1:
      if (dataIn[14])
      {
        topDoorStop();
        topDoorState = 0;
        topDoorOpr = 0;
        topDoorStep ++;
      }
      break;
    }
  }
  
}

void emFrontDoor()
{
  if((topDoorState == 0) & (topDoorStep >=2))
  {
    frontDoorStep = 0;
    frontDoorOpr =1;
  }
  
}

void getToolProcess()
{
  switch (ejectStep)
  {
    case 0: //Open Top Door
      openTopDoor();
      ejectStep ++;
      break;

    case 1://Eject Tool
      if (topDoorState == 1)
      {
        timerDetectTool =0;
        monitorStep = 0; //reset Drop Monitor Process

        spiralMotorStep =0;
        spiralTimer =0;

        ejectStep ++;
      }
      break;
  }
}

void dropMonitor() 
{

  if((timerDetectTool>25) & (monitorStep <2))
    {
      monitorStep =2;
      Serial.println("124");//Action Fail
      closeTopDoor();
      //digitalWrite(endrop, HIGH);
    }
  switch (monitorStep)
  {
    case 0 :
      if (dataIn[11] != lastDataIn[11]) 
      {
        if (dataIn[11])
        {
          Serial.println("123");
          closeTopDoor();
          monitorStep ++;
        }
      
      }
      lastDataIn[11] = dataIn[11];
      break;
    case 1: 
      if (topDoorState == 0)    
      {
        openFrontDoor();
        monitorStep ++;
      }
      break;
  }
}


void spiralMotorProcess()
{
  if((spiralMotorStep < 2) & (spiralTimer > 25) )
  {
    spiralMotorStep =2;
    stopSpiral();
  }
  switch (spiralMotorStep)
  {
  case 0:
    rotateSpiral(slotNumber);
    spiralSWCounter = 0;
    spiralMotorStep ++;
    break;
  case 1:
    if (dataIn[fbID] != lastDataIn[fbID])
    {
      if(dataIn[fbID] == LOW)
      {
        detectPosition = true;
        timerDetectPosition = 0;
      }
      
    }
    if(detectPosition & (timerDetectPosition > 2))
    {
      detectPosition = false;
      stopSpiral();
      spiralMotorStep ++;
    }
    lastDataIn[fbID] = dataIn[fbID];

    break;
  }

}


void setMotorOuputStatus(int slot)
{
  int row, column;
  if(slot > 0 and slot <= 60)
  {
    row = (slot -1)/10;
    column = (slot -1)%10;
    fbID = 7 - row ;
    for(int i = 0; i<10;i++)
    {
      if(i== column) dataOut[i] = 1;
      else dataOut[i] = 0;
    }
    for(int i = 10; i<16;i++){
      if((i-10) == row) dataOut[i] = 1;
      else dataOut[i] = 0;
    }
  }
}

void shiftOutData(bool data[], int length)
{
  for(int i =0; i<length; i++)
  {
      digitalWrite(DS, dataOut[length-i -1]);
      digitalWrite(SHCP, LOW);
      digitalWrite(SHCP, HIGH);
  }
  digitalWrite(STCP, LOW);
  digitalWrite(STCP, HIGH);
}

void timerTick()
{
  if (monitorStep < 2)
  {
    timerDetectTool ++;
  }
  
  if (frontDoorState) 
  {
    timerFrontDoor ++;
  }

  if(spiralMotorStep <2)
  {
    spiralTimer++;
  }

  if(detectPosition)
  {
    timerDetectPosition ++;
  }

  if ((timerFrontDoor >= delayOpenTime) & (frontDoorState == 1))
  {
    timerFrontDoor =0;
    frontDoorStep =0;
    frontDoorOpr = 2;
  }
}

void stopSpiral()
{
  for(int i =0;i<16;i++)
  {
      dataOut[i]=0;
  }
  shiftOutData(dataOut, 24);
}

void openFrontDoor()
{
  frontDoorOpr = 1;
  frontDoorStep =0;
}

void closeFrontDoor()
{
  frontDoorOpr = 2;
  frontDoorStep =0;
}

void openTopDoor()
{
  topDoorOpr = 1;
  topDoorStep =0;
}
void closeTopDoor()
{
  topDoorOpr = 2;
  topDoorStep =0;
}

void frontDoorRev()
{
  digitalWrite(enA, HIGH);
  dataOut[16] = 0;
  dataOut[17] = 1;
  shiftOutData(dataOut, 24);
}

void frontDoorFwd()
{
  digitalWrite(enA, HIGH);
  dataOut[16] = 1;
  dataOut[17] = 0;
  shiftOutData(dataOut, 24);
}

void topDoorRev()
{
  digitalWrite(enB, HIGH);
  dataOut[18] = 0;
  dataOut[19] = 1;
  shiftOutData(dataOut, 24);
}

void topDoorFwd()
{
  digitalWrite(enB, HIGH);
  dataOut[18] = 1;
  dataOut[19] = 0;
  shiftOutData(dataOut, 24);
}

void topDoorStop()
{
  digitalWrite(enB, LOW);
  dataOut[18] = 0;
  dataOut[19] = 0;
  shiftOutData(dataOut, 24);
}

void frontDoorStop()
{
  digitalWrite(enA, LOW);
  dataOut[16] = 0;
  dataOut[17] = 0;
  shiftOutData(dataOut, 24);
}