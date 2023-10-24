// -------------- IMPORTATION OF LIBRARIES --------------
#include <stdio.h>     
#include <math.h>       
#include "DHT.h"
#include <SPI.h>
#include <mcp2515.h>


// (1)-------------- GENERAL SET-UP --------------(1)
uint32_t initialTime;
uint32_t sendDataTimer;
uint32_t sendDataTimeout = 1000;

bool canDataRelevant = false;
bool dhtDataRelevant = false;
bool RtDataRelevant  = true;



// (2)-------------- DHT SET-UP AND RELATED FUNCS --------------(2)
#define DHTPIN 3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
const int NUM_THdht_MEMORY = 2;

float Tdht;
float Hdht;
float Tdht_nanmean;
float Hdht_nanmean;
float Tdht_memory[NUM_THdht_MEMORY];
float Hdht_memory[NUM_THdht_MEMORY];



void readUpdateTHdht(){
  Tdht = dht.readTemperature();
  Hdht = dht.readHumidity();
}

void updateTHdht_memory(){
  for(int i=0; i<NUM_THdht_MEMORY-1; i++){
  Tdht_memory[i] = Tdht_memory[i+1];
  Hdht_memory[i] = Hdht_memory[i+1];
  }

  Tdht_memory[NUM_THdht_MEMORY-1] = Tdht;
  Hdht_memory[NUM_THdht_MEMORY-1] = Hdht;
}


void updateTHdht_nanmean(){

  float sum1 = 0;
  float sum2 = 0;
  int count1 = 0;
  int count2 = 0;

  for(int i=0; i<NUM_THdht_MEMORY; i++){

    if (!isnan(Tdht_memory[i])) {
    sum1 += Tdht_memory[i];
    count1++;
    }

    if (!isnan(Hdht_memory[i])) {
    sum2 += Hdht_memory[i];
    count2++;
    }

    if (count1 == 0) {Tdht_nanmean = NAN;}
    else             {Tdht_nanmean = sum1 / count1;}

    if (count2 == 0) {Hdht_nanmean = NAN;}
    else             {Hdht_nanmean = sum2 / count2;}
  }
}




// (3)-------------- THERMISTOR SET-UP AND RELATED FUNCS --------------(3)

const int NUM_RT_PINS          = 5;
const int RT_PINS[NUM_RT_PINS] = {A2, A3, A4, A5, A6};
float     R1[NUM_RT_PINS]      = {31700.0, 31280.0, 31860.0, 31580.0, 31265.0};
const int NUM_Tt_MEMORY        = 10;

float Vcc = 5.0;
float Rt[NUM_RT_PINS];
float Vt[NUM_RT_PINS];
float Tt[NUM_RT_PINS];
float Tt_nanmean[NUM_RT_PINS];
float Tt_memory[NUM_Tt_MEMORY][NUM_RT_PINS];


void readUpdateVt(){
  for(int i=0; i<NUM_RT_PINS; i++){
    Vt[i] = Vcc*analogRead(RT_PINS[i])/1023.0;
  }
}

void updateRt(){
  for(int i=0; i<NUM_RT_PINS; i++){
    Rt[i] = R1[i]*Vt[i]/(Vcc - Vt[i]);
  }
}

void updateTt(){
  float lnRt;
  float A =  610.5282190830485;
  float B = -85.97402403405586;
  float C =  3.4037509247172553;
  float D = -0.007982876975644648;
  float E = -0.0019710091731159445;

  for(int i=0; i<NUM_RT_PINS; i++){
  lnRt = log(Rt[i]);
  Tt[i] = A + B*lnRt + C*pow(lnRt,2) + D*pow(lnRt,3) + E*pow(lnRt,4);
  }
}

void updateTt_memory(){
  for(int i=0; i<NUM_Tt_MEMORY-1; i++){for(int j=0; j<NUM_RT_PINS; j++){Tt_memory[i][j] = Tt_memory[i+1][j];}}

  for(int j=0; j<NUM_RT_PINS; j++){
    Tt_memory[NUM_Tt_MEMORY-1][j] = Tt[j];
    }
}

void updateTt_nanmean(){
  for(int j=0; j<NUM_RT_PINS; j++){

    float sum = 0;
    int count = 0;
    for(int i=0; i<NUM_Tt_MEMORY; i++){
      if (!isnan(Tt_memory[i][j])) {
      sum += Tt_memory[i][j];
      count++;
      }
    }

    if (count == 0) {Tt_nanmean[j] = NAN;}
    else            {Tt_nanmean[j] = sum / count;}
  }
}


// (4)-------------- CAN SET-UP --------------(4)
struct can_frame canMsg;
MCP2515 mcp2515(10);

float batteryVoltage = NAN;
float batteryCurrent = NAN;
float batterySOC = NAN;

void readUpdateBatteryData(){
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if(canMsg.can_id == 2550245121){
      batteryVoltage = 0.1*(  (canMsg.data[1] << 8) | canMsg.data[0]);
      batteryCurrent = 0.1*( ((canMsg.data[3] << 8) | canMsg.data[2]) - 32000);
      batterySOC     = 1.0*canMsg.data[4];
    }
  }
}





// (5)-------------- PRINT DATA --------------(5)

void excelSetup(){

  String setupString = "LABEL, Date, Time, Milliseconds";

  if(canDataRelevant){
    setupString = setupString + ",V,I,Soc";} 

  if(dhtDataRelevant){
    setupString = setupString + ",Tdht,Hdht";}

  if(RtDataRelevant){
    for(int i=0; i<NUM_RT_PINS; i++){
      setupString = setupString + ",Tt" + i;}
  }

  Serial.println("CLEARDATA");
  Serial.println("CLEARSHEET");
  Serial.println(setupString);
  Serial.println("RESETTIMER");
}

void sendData(){
  String dataString = "DATA,DATE,TIME";
  dataString = dataString + "," + (millis() - initialTime);

  if(canDataRelevant){
    dataString = dataString + "," + batteryVoltage;
    dataString = dataString + "," + batteryCurrent;
    dataString = dataString + "," + batterySOC;
  }

  if(dhtDataRelevant){
    updateTHdht_nanmean();
    dataString = dataString + "," + Tdht_nanmean; 
    dataString = dataString + "," + Hdht_nanmean; 
  }

  if(RtDataRelevant){
    updateTt_nanmean();
    for(int i=0; i<NUM_RT_PINS; i++){
      dataString = dataString + "," + Tt_nanmean[i] ;
    }
  }

  Serial.println(dataString);

}




void setup() {
  // SET THERMISTOR PINS TO INPUT.
  
  for(int i=0; i<NUM_RT_PINS; i++){
    pinMode(RT_PINS[i], INPUT);
  }

  // INITIALIZE SERIAL AND DHT SENSOR.
  Serial.begin(9600);
  while (!Serial){;}
  dht.begin();

  // INITIALIZE CAN MODULE
  mcp2515.reset();
  mcp2515.setBitrate(CAN_250KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();




  // INITIALIZE MEMORY ARRAYS TO NAN.
  for(int i=0; i<NUM_THdht_MEMORY; i++){Tdht_memory[i] = NAN; Hdht_memory[i] = NAN;}
  for(int i=0; i<NUM_Tt_MEMORY; i++){for(int j=0; j<NUM_RT_PINS; j++){Tt_memory[i][j] = NAN;}}

  // INITIALIZE TIME VARIABLES.
  initialTime   = millis();
  sendDataTimer = millis();

  // EXECUTE EXCEL SET-UP.
  excelSetup();

}



void loop() {

  // BATTERY CAN DATA
  readUpdateBatteryData();
  
  // DHT SENSOR
  readUpdateTHdht();
  updateTHdht_memory();

  // THERMISTORS
  readUpdateVt();
  updateRt();
  updateTt();
  updateTt_memory();


  // SEND DATA OVER TO EXCEL
  if(millis() - sendDataTimer > sendDataTimeout){
    sendData();
    sendDataTimer = millis();
    }

    
  }
