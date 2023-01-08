#include <SoftwareSerial.h>
//#define RL 47
//#define Ro 129
//#define m -0.263
//#define b 0.42
#define RL 20
#define Ro 4.82
#define m -0.263
#define b 0.42
#define pHPin A2
#define nhPin A3
#define offset 7.31 //3.04
#define LED 13  
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth  40
int pHArray[ArrayLenth];
int pHArrayIndex=0;
float ppm, ntu, pHValue;
String dataRead;
SoftwareSerial DataSerial(0,2); //rx tx;

void setup() {
  Serial.begin(9600);
  DataSerial.begin(9600); 
}

void loop() {
  bacaKekeruhan();
  bacaAmonia();
  bacaPH();
  String espRequestData = "";
  while(Serial.available()>0){
    espRequestData += char(Serial.read());
  }
  espRequestData.trim();
  if(espRequestData == "Ya"){
    kirimData();
  }
  espRequestData = "";
  delay(1000);
}

void bacaKekeruhan(){
  int turbSensorValue = analogRead(A0);
  float voltage = turbSensorValue * (5.0 / 1024.0);
//  ntu = -26.65 * voltage + 117.5;
  ntu = abs(-33.99 * voltage + 129.57);
  Serial.print("voltage turbidity: ");
  Serial.println(voltage);
  Serial.print("ntu: ");
  Serial.println(ntu);
}

void bacaAmonia(){
  float Rs;
  float ratio;
  int nhSensorValue = analogRead(nhPin);
  float voltage = nhSensorValue * (5.0 / 1023.0);
  Rs = ((5.0*RL)/voltage)-RL;
  ratio = Rs/Ro;
  ppm = pow(10, ((log10(ratio)-b)/m));
  Serial.print("ppm test: ");
  Serial.println(ppm);
}

void bacaPH(){
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float voltage;
  if(millis()-samplingTime > samplingInterval)
  {
      pHArray[pHArrayIndex++]=analogRead(pHPin);
      if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLenth)*5.0/1024;
      pHValue = 0.29 * voltage + offset;
      samplingTime=millis();
  }
  Serial.print("pH: ");
  Serial.println(pHValue);
  Serial.print("voltage pH: ");
  Serial.println(voltage);
}

void kirimData(){
  dataRead = String(ppm) + '#' + String(ntu) + '#' + String(pHValue);
  DataSerial.println(dataRead);
  Serial.println(dataRead);
}


double avergearray(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}

float mapper(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
