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

bool RtDataRelevant  = true;
bool CurrentRelevant = false;

const int NUM_PCBS    = 4;
const int NUM_THERMS  = 16;


const float Vcc[NUM_PCBS]            = {5.013, 5.013, 5.013, 5.013};

const float R1[NUM_PCBS][NUM_THERMS] = {{33100, 33150, 33180, 33070, 33010, 33050, NAN,   NAN,   NAN,   NAN,   NAN,   NAN,   NAN,   NAN},
                                        {33990, 32850, 32350, 32480, 32720, 33160, 32710, 33130, 33040, 32710, 32730, 32700, 33240, 33010},
                                        {32940, 33000, 33190, 32780, 32870, 33140, 33050, 32860, 32980, 32330, 32730, 32830, 33100, 33090},
                                        {33110, 32670, 33030, 33220, 32480, 32440, 32850, 33010, 33040, 32950, 32700, 32760, 32980, 32620}};

const float RT_PINS[NUM_PCBS][NUM_THERMS] = {{A0, A1, A2, A3, A4, A5, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN},
                                             {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13},
                                             {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13},
                                             {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13}};

float V1[NUM_THERMS] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
float Vt[NUM_THERMS] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
float I[NUM_THERMS]  = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
float Rt[NUM_THERMS] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
float Tt[NUM_THERMS] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};



// (2)-------------- SPECIFIC SET-UP --------------(2)
const int NUM_PCB = 0;
const intNUM_AVGS = 20;

// (3)-------------- SPECIFIC SET-UP --------------(3)



void readUpdate_V1_Vt_I_Rt(){
  for(int i=0; i<NUM_THERMS; i++){
  if(!isnan(RT_PINS[NUM_PCB][i])){

    V1[i] = 0;
    for(int j=0; j<NUM_AVGS; j++){
      V1[i] += analogRead(RT_PINS[i]);}

    V1 *= Vcc[NUM_PCB]
    
   }}

   


  
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
float NTC[7] =  {NAN, NAN, NAN, NAN, NAN, NAN, NAN};

void readUpdateBatteryData(){

  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if(canMsg.can_id == 2550245121){
      batteryVoltage = 0.1*(  (canMsg.data[1] << 8) | canMsg.data[0]);
      batteryCurrent = 0.1*( ((canMsg.data[3] << 8) | canMsg.data[2]) - 32000);
      batterySOC     = 1.0*canMsg.data[4];
      
    }else if(canMsg.can_id == 2550834945){
      NTC[1] = canMsg.data[4] - 40;
      NTC[2] = canMsg.data[5] - 40;
      NTC[3] = canMsg.data[6] - 40;
      NTC[4] = canMsg.data[7] - 40;
      
    }else if(canMsg.can_id == 2550900481){
      NTC[5] = canMsg.data[0] - 40;
      NTC[6] = canMsg.data[1] - 40;
      NTC[0] = canMsg.data[2] - 40;
    }
  }
}




// (5)-------------- PRINT DATA --------------(5)

void excelSetup(){

  String setupString = "LABEL, Date, Time, Milliseconds";

  if(canDataRelevant){
    setupString = setupString + ",V,I,Soc";} 
    for(int i=0; i<6; i++){setupString = setupString + ",NTC" + i;}

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
    for(int i=0; i<6; i++){dataString = dataString + "," + NTC[i];}
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
