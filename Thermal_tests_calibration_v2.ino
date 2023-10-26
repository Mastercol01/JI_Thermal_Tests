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
const int NUM_THdht_AVGS = 10;

float Tdht;
float Hdht;

void readUpdateTHdht(){
  Tdht = 0;
  Hdht = 0;

  for(int i=0; i<NUM_THdht_AVGS; i++){
    Tdht += dht.readTemperature();
    Hdht += dht.readHumidity();
    }
  
  Tdht /= NUM_THdht_AVGS;
  Hdht /= NUM_THdht_AVGS;
}




// (3)-------------- THERMISTOR SET-UP AND RELATED FUNCS --------------(3)

const int NUM_RT_PINS          = 5;
const int RT_PINS[NUM_RT_PINS] = {A2, A3, A4, A5, A6};
float     R1[NUM_RT_PINS]      = {32980.0, 32470.0, 33360.0, 33000.0, 32550.0};
const int NUM_Tt_AVGS          = 35;

float Vcc = 5.012;
float Rt[NUM_RT_PINS];
float Vt[NUM_RT_PINS];
float Tt[NUM_RT_PINS];




void readUpdateVt(){
  for(int i=0; i<NUM_RT_PINS; i++){
    Vt[i] = 0;
    for(int j=0; j<NUM_Tt_AVGS; j++){
      Vt[i] += analogRead(RT_PINS[i]);
      }
      Vt[i] /= NUM_Tt_AVGS;
      Vt[i] = Vcc*Vt[i]/1023.0;
  }
}

void updateRt(){
  for(int i=0; i<NUM_RT_PINS; i++){
    Rt[i] = R1[i]*Vt[i]/(Vcc - Vt[i]);
  }
}


void updateTt_v1(){
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

void updateTt_v2(){
  float lnRt;
  float Tt_inverse;
  float A  =  0.00211513;
  float B  =  0.000281579;
  float C  = -6.2803*pow(10,-7);
  
  for(int i=0; i<NUM_RT_PINS; i++){
  lnRt       = log(Rt[i]/1000);
  Tt_inverse = A + B*lnRt + C*pow(lnRt,3);
  Tt[i]      = 1/Tt_inverse - 273.15;
  }
}

void updateTt_v3(){
  float Tt_inverse;
  float B  = 3950;
  float T0 = 25 + 273.15;
  float R0 = 100000;

  
  for(int i=0; i<NUM_RT_PINS; i++){
  Tt_inverse = 1/T0 + 1/B*log(Rt[i]/R0);
  Tt[i]      = 1/Tt_inverse - 273.15;
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
    for(int i=0; i<NUM_RT_PINS; i++){setupString = setupString + ",Vt" + i;}
    for(int i=0; i<NUM_RT_PINS; i++){setupString = setupString + ",Rt" + i;}
    for(int i=0; i<NUM_RT_PINS; i++){setupString = setupString + ",Tt" + i;}    
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
    dataString = dataString + "," + Tdht; 
    dataString = dataString + "," + Hdht; 
  }

  if(RtDataRelevant){
    for(int i=0; i<NUM_RT_PINS; i++){dataString = dataString + "," + Vt[i];}
    for(int i=0; i<NUM_RT_PINS; i++){dataString = dataString + "," + Rt[i];}
    for(int i=0; i<NUM_RT_PINS; i++){dataString = dataString + "," + Tt[i];}
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

  // THERMISTORS
  readUpdateVt();
  updateRt();
  updateTt_v1();


  // SEND DATA OVER TO EXCEL
  if(millis() - sendDataTimer > sendDataTimeout){
    sendData();
    sendDataTimer = millis();
    }

    
  }
