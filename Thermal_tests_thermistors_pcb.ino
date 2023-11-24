// -------------- IMPORTATION OF LIBRARIES --------------
#include <stdio.h>     
#include <math.h>       
#include <SPI.h>
#include <Adafruit_ADS1X15.h>

// (1)-------------- GENERAL SET-UP --------------(1)
uint32_t initialTime;
uint32_t sendDataTimer;
uint32_t sendDataTimeout = 1500;

Adafruit_ADS1115 ads;                    /* Use this for the 16-bit version */
const float MULTIPLIER = 0.03125*1e-3;   // for ads.setGain(GAIN_FOUR);   // 4x gain   +/- 1.024V  1 bit = 0.03125mV (ADS1115)
const float FACTOR     = 25;             // 20A/1V from the current transformer

bool RtDataRelevant   = true;
bool IrmsDataRelevant = false;

const int NUM_PCBS    = 4;
const int NUM_THERMS  = 14;


const float Vcc[NUM_PCBS]            = {5.013, 4.98, 5.013, 5.013};

const float R1[NUM_PCBS][NUM_THERMS] = {{33100, 33150, 33180, 33070, 33010, 33050, NAN,   NAN,   NAN,   NAN,   NAN,   NAN,   NAN,   NAN},
                                        {33990, 32850, 32350, 32480, 32720, 33160, 32710, 33130, 33040, 32710, 32730, 32700, 33240, 33010},
                                        {32940, 33000, 33190, 32780, 32870, 33140, 33050, 32860, 32980, 32330, 32730, 32830, 33100, 33090},
                                        {33110, 32670, 33030, 33220, 32480, 32440, 32850, 33010, 33040, 32950, 32700, 32760, 32980, 32620}};

const float RT_PINS[NUM_PCBS][NUM_THERMS] = {{A0, A1, A2, A3, A4, A5, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN},
                                             {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, A12, A13},
                                             {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13},
                                             {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13}};

float V1[NUM_THERMS] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
float Vt[NUM_THERMS] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
float I[NUM_THERMS]  = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
float Rt[NUM_THERMS] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
float Tt[NUM_THERMS] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};




// (2)-------------- SPECIFIC SET-UP --------------(2)
const int NUM_PCB = 1;
const int NUM_AVGS = 20;


// (3)-------------- THERMISTOR SENSOR FUNCS --------------(3)
void readUpdate_V1_Vt_I_Rt(){

  for(int i=0; i<NUM_THERMS; i++){
  if(!isnan(RT_PINS[NUM_PCB][i])){

    V1[i] = 0;
    for(int j=0; j<NUM_AVGS; j++){
      V1[i] += analogRead(RT_PINS[NUM_PCB][i]);
    }

    V1[i] *= (Vcc[NUM_PCB]/1023)/NUM_AVGS;
    Vt[i]  = Vcc[NUM_PCB] - V1[i];
    I[i]   = V1[i]/R1[NUM_PCB][i];
    Rt[i]  = Vt[i]/I[i];
   }}
}


void updateTt_v1(){
  float lnRt;
  float A =  610.5282190830485;
  float B = -85.97402403405586;
  float C =  3.4037509247172553;
  float D = -0.007982876975644648;
  float E = -0.0019710091731159445;

  for(int i=0; i<NUM_THERMS; i++){
  if(!isnan(RT_PINS[NUM_PCB][i])){

    lnRt = log(Rt[i]);
    Tt[i] = A + B*lnRt + C*pow(lnRt,2) + D*pow(lnRt,3) + E*pow(lnRt,4);
  }}
}


void updateTt_v2(){
  float lnRt;
  float Tt_inverse;
  float A  =  0.00211513;
  float B  =  0.000281579;
  float C  = -6.2803*pow(10,-7);

  for(int i=0; i<NUM_THERMS; i++){
  if(!isnan(RT_PINS[NUM_PCB][i])){

    lnRt       = log(Rt[i]/1000);
    Tt_inverse = A + B*lnRt + C*pow(lnRt,3);
    Tt[i]      = 1/Tt_inverse - 273.15;
  }}
}


void updateTt_v3(){
  float Tt_inverse;
  float B  = 3950;
  float T0 = 25 + 273.15;
  float R0 = 100000;

  for(int i=0; i<NUM_THERMS; i++){
  if(!isnan(RT_PINS[NUM_PCB][i])){

    Tt_inverse = 1/T0 + 1/B*log(Rt[i]/R0);
    Tt[i]      = 1/Tt_inverse - 273.15;
  }}
}





// (4)-------------- CURRENT TRANSFORMER FUNCS --------------(4)
float SumIsq = 0;
float SumIsqNumSamples = 0;
float Irms = 0;

void readUpdate_SumIsq_SumIsqNumSamples_Irms(){

  float bitVoltage    = ads.readADC_Differential_0_1();
  float voltage       = MULTIPLIER*bitVoltage;
  float current       = FACTOR*voltage;
  SumIsq             += pow(current, 2);
  SumIsqNumSamples   += 1;
  Irms                = sqrt(SumIsq/SumIsqNumSamples);
}


void reset_SumIsq_SumIsqNumSamples_Irms(){
  SumIsq = 0;
  SumIsqNumSamples = 0;
  Irms = 0;
}




// (5)-------------- PRINT DATA --------------(5)

void excelSetup(){

  String setupString = "LABEL, Date, Time, Milliseconds";

  if(IrmsDataRelevant){
    setupString = setupString + ",Irms";
  }

  if(RtDataRelevant){
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){setupString = setupString + ",V1_" + "(" + NUM_PCB + "," + i + ")";}}
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){setupString = setupString + ",Vt_" + "(" + NUM_PCB + "," + i + ")";}}
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){setupString = setupString + ",I_"  + "(" + NUM_PCB + "," + i + ")";}}
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){setupString = setupString + ",Rt_" + "(" + NUM_PCB + "," + i + ")";}}    
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){setupString = setupString + ",Tt_" + "(" + NUM_PCB + "," + i + ")";}}  
  }

  Serial.println("CLEARDATA");
  Serial.println("CLEARSHEET");
  Serial.println(setupString);
  Serial.println("RESETTIMER");
}


void sendData(){
  String dataString = "DATA,DATE,TIME";
  dataString = dataString + "," + (millis() - initialTime);

  if(IrmsDataRelevant){
    dataString = dataString + "," + Irms;
  }

  if(RtDataRelevant){
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){dataString = dataString + "," + V1[i];}}
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){dataString = dataString + "," + Vt[i];}}
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){dataString = dataString + "," + I[i];}}
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){dataString = dataString + "," + Rt[i];}}
    for(int i=0; i<NUM_THERMS; i++){if(!isnan(RT_PINS[NUM_PCB][i])){dataString = dataString + "," + Tt[i];}}
  }

  Serial.println(dataString);

}




void setup() {

  // SET THERMISTOR PINS TO INPUT.
  for(int i=0; i<NUM_THERMS; i++){
  if(!isnan(RT_PINS[NUM_PCB][i])){

    pinMode(RT_PINS[NUM_PCB][i], INPUT);
  }}
  
  // INITIALIZE SERIAL.
  Serial.begin(9600);
  while (!Serial){;}

  // INITIALIZE ADS1115 SENSOR.
  if(IrmsDataRelevant){
  if (!ads.begin()){
    Serial.println("Failed to initialize ADS."); 
    while (1);
  }
  ads.setGain(GAIN_FOUR); 
  }                         

  // INITIALIZE TIME VARIABLES.
  initialTime   = millis();
  sendDataTimer = millis();

  // EXECUTE EXCEL SET-UP.
  excelSetup();

}




void loop() {

  // READ CURRENT SENSOR
  if(IrmsDataRelevant){
    readUpdate_SumIsq_SumIsqNumSamples_Irms();
  }


  // THERMISTORS
  if(RtDataRelevant){
    readUpdate_V1_Vt_I_Rt();
    updateTt_v1();
  }


  // SEND DATA OVER TO EXCEL
  if(millis() - sendDataTimer > sendDataTimeout){
    sendData();
    reset_SumIsq_SumIsqNumSamples_Irms();
    sendDataTimer = millis();
  }

}
