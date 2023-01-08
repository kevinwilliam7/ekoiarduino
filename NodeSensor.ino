#include <IOXhop_FirebaseESP32.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
const int oneWireBus = 13;
#define TRIG 12
#define ECHO 27
#define TRIG2 26  
#define ECHO2 25
#define FIREBASE_HOST "https://smartaquariumcopy-d70d0-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "hbB2dCnXMZlZTMJb9hLG4x8Eh82OkyLj9mqNHpAb"
unsigned long interval=3600000;
unsigned long previousMillis=0;
const char *ssid = "OPPOA31";
const char *pass = "ribet2020";
float distance, duration, distance2, duration2, pHValue, turbidityValue, nhValue, temperatureC, foodLevel, waterLevel;
float tinggiWadah = 20.15;
float tinggiAkuarium;
//float tinggiAkuarium = 46.8;
String dateRead, timeRead;
const long utcOffsetInSeconds = 25200;
WiFiServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);
SoftwareSerial DataSerial(32,15); //rx tx;
OneWire oneWire(oneWireBus);
DallasTemperature sensorTemp(&oneWire);
String arrData[3];

void setup() {
  Serial.begin(9600);
  DataSerial.begin(9600);
  pinMode(ECHO,INPUT);
  pinMode(TRIG,OUTPUT);
  pinMode(ECHO2,INPUT);
  pinMode(TRIG2,OUTPUT);
  WiFi.begin(ssid, pass);
  uint32_t notConnectedCounter = 0;
  while(WiFi.status() != WL_CONNECTED){
    delay(100);
    Serial.println("Wifi connecting...");
    notConnectedCounter++;
    if(notConnectedCounter > 150) { // Reset board if not connected after 5s
        Serial.println("Resetting due to Wifi not connecting...");
        ESP.restart();
    }
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  timeClient.begin();
  tinggiAkuarium = Firebase.getFloat("konfigurasiKontrol/tinggi_akuarium");
}

void loop() {
  timeClient.update(); //melakukan update waktu pada timeClient
  String formattedDate = timeClient.getFormattedDate(); //memanggil formattedDate dari lib timeClient
  int splitT = formattedDate.indexOf("T"); //mengambil format DD/MM/YY pada formattedDate
  dateRead = formattedDate.substring(0, splitT); //mengubah format formattedDate dari lib timeClient
  timeRead = timeClient.getFormattedTime();
  tinggiair();
  tinggiPakan();
  bacaSuhu();
  requestDataArduino();
//  storeFirebase();
}

void tinggiair(){
  digitalWrite(TRIG,LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG,HIGH);
  delayMicroseconds(20);
  digitalWrite(TRIG,LOW);
  duration = pulseIn(ECHO,HIGH);
  distance = (duration / 2) * 0.034;
  waterLevel = tinggiAkuarium - distance;
  if(waterLevel<=0){Firebase.set("sensor/tinggi_air", 0);}
  else{Firebase.set("sensor/tinggi_air", waterLevel);}\
  Serial.print("tinggi air: ");
  Serial.println(waterLevel);
}

void tinggiPakan(){
  digitalWrite(TRIG2,LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG2,HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG2,LOW);
  duration2 = pulseIn(ECHO2,HIGH);
  distance2 = (duration2 / 2) * 0.034;
  foodLevel = tinggiWadah - distance2;
  if(foodLevel<=0){Firebase.set("sensor/tinggi_pakan", 0);}
  else{Firebase.set("sensor/tinggi_pakan", foodLevel);}
  Serial.print("tinggi pakan: ");
  Serial.println(foodLevel);
}

void bacaSuhu(){
  sensorTemp.requestTemperatures(); 
  temperatureC = sensorTemp.getTempCByIndex(0);
  if(temperatureC==-127||temperatureC>=80){}
  else{Firebase.set("sensor/suhu", temperatureC);}
  Serial.print("suhu: ");
  Serial.println(temperatureC);
}

void requestDataArduino(){
  String dataFromArduino = "";
  DataSerial.println("Ya");
  while(DataSerial.available()>0){
    dataFromArduino += char(DataSerial.read());
  }
  dataFromArduino.trim();
  if(dataFromArduino!=""){
    int index = 0;
    for(int i=0; i<dataFromArduino.length(); i++){
      char separated = '#';
      if(dataFromArduino[i] != separated){
        arrData[index] += dataFromArduino[i];
      }
      else{
        index++;
      }
    }
    nhValue = arrData[0].toFloat();
    turbidityValue = arrData[1].toFloat();
    pHValue = arrData[2].toFloat();
    //ppm#ntu#ph
    Firebase.set("sensor/kekeruhan", turbidityValue);
    Firebase.set("sensor/amonia", nhValue);
    if(pHValue>0){
      Firebase.set("sensor/ph", pHValue);
    }
    arrData[0] = "";
    arrData[1] = "";
    arrData[2] = "";
  }
}

void storeFirebase(){
  if(millis() - previousMillis >= interval) 
  {
    StaticJsonBuffer<1000> jsonBuffer;
    JsonObject& objectData = jsonBuffer.createObject();
    objectData["tinggi_air"]=waterLevel;
    objectData["tinggi_pakan"]=foodLevel;
    objectData["suhu"]=temperatureC;
    objectData["kekeruhan"]=turbidityValue;
    objectData["amonia"]=nhValue;
    objectData["ph"]=pHValue;
    objectData["time"]=timeRead;
    objectData["date"]=dateRead;
    Firebase.push("data_sensor", objectData);
    previousMillis = millis();
  }
}
