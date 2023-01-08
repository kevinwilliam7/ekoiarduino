#include <IOXhop_FirebaseESP32.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <Servo.h> 
#define FIREBASE_HOST "https://smartaquariumcopy-d70d0-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "hbB2dCnXMZlZTMJb9hLG4x8Eh82OkyLj9mqNHpAb"
const char *ssid = "OPPOA31";
const char *pass = "ribet2020";
const int pinPakan = 33; 
const int pinUV = 18; 
const int pinPumpCooler = 19;
const int pinHeater = 27; 
const int pinCooler = 26;
const int pinDrain = 32;
const int pinPump = 13;
int buka = 30;
int tutup = 0;
String dateRead, timeRead;
int hourCurrent, minuteCurrent;
Servo myservo;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 25200);
WiFiServer server(80);
StaticJsonBuffer<1000> jsonBuffer;
JsonObject& objectData = jsonBuffer.createObject();
int jamPakan1, jamPakan2, statusPakan1, menitPakan1, menitPakan2, statusPakan2;
float suhuMaksimum, suhuMinimum, keruhMaks, phMaks, phMin, amoniaMaks, airMaks, airMin, readTinggiPakan, pakanMin;
float readSuhu, readAmonia, readPH, readKekeruhan, readTinggiAir;
bool modeAlat, switchUV, switchCooler, switchHeater, switchPump, switchDrain;

void setup() {
  Serial.begin(9600);
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
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(pinHeater, 1); //matikan relay heater
  digitalWrite(pinDrain, 1); //matikan relay penguras
  digitalWrite(pinPump, 1); //matikan relay pengisi
  digitalWrite(pinCooler, 1); //matikan relay cooler
  digitalWrite(pinUV, 1); //matikan relay uv
  digitalWrite(pinPumpCooler, 1); //matikan relay cooler
  pinMode(pinHeater, OUTPUT);
  pinMode(pinCooler, OUTPUT);
  pinMode(pinDrain, OUTPUT);
  pinMode(pinPump, OUTPUT);
  pinMode(pinUV, OUTPUT);
  pinMode(pinPumpCooler, OUTPUT);
  myservo.attach(pinPakan);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  timeClient.begin();
  Firebase.stream("/control/servo/mode", [](FirebaseStream stream) {
    String eventType = stream.getEvent();
    String path = stream.getPath();
    String data = stream.getDataString();
    eventType.toLowerCase();
    if ((eventType=="put" || eventType=="patch")) {
      modeAlat = stream.getDataBool();
    }
  });  
}

void loop() {
  timeClient.update(); 
  String formattedDate = timeClient.getFormattedDate(); //memanggil formattedDate dari lib timeClient
  int splitT = formattedDate.indexOf("T"); //mengambil format DD/MM/YY pada formattedDate
  dateRead = formattedDate.substring(0, splitT); //mengubah format formattedDate dari lib timeClient
  timeRead = timeClient.getFormattedTime();
  hourCurrent = timeClient.getHours();
  minuteCurrent = timeClient.getMinutes();
  resetJadwalPakan();
  if(modeAlat==1){
    kendaliPakan();
    kendaliSuhu();
    filter();
    kendaliAir();
  }
  else{
    digitalWrite(pinCooler, relayBool(Firebase.getBool("control/cooler/value")));
    digitalWrite(pinPumpCooler, relayBool(Firebase.getBool("control/cooler/value")));
    digitalWrite(pinHeater, relayBool(Firebase.getBool("control/heater/value")));
    digitalWrite(pinDrain, relayBool(Firebase.getBool("control/drain/value")));
    digitalWrite(pinPump, relayBool(Firebase.getBool("control/pump/value")));
    digitalWrite(pinUV, relayBool(Firebase.getBool("control/uv/value")));
    if(Firebase.getBool("control/servo/value")==1 && modeAlat==0){
      myservo.write(buka);
      delay(800);
      myservo.write(tutup);
      Firebase.set("control/servo/value", false);
    } 
  }
}

void resetJadwalPakan(){
  if(hourCurrent==00){
    Firebase.set("jadwal_pakan/pkn1/status", false);
    Firebase.set("jadwal_pakan/pkn2/status", false);
  }
}

void kendaliPakan(){
  readTinggiPakan = Firebase.getFloat("sensor/tinggi_pakan");
  jamPakan1 = Firebase.getInt("jadwal_pakan/pkn1/jam");
  menitPakan1 = Firebase.getInt("jadwal_pakan/pkn1/menit");
  jamPakan2 = Firebase.getInt("jadwal_pakan/pkn2/jam");
  menitPakan2 = Firebase.getInt("jadwal_pakan/pkn2/menit");
  pakanMin = Firebase.getFloat("konfigurasiKontrol/pakan_min");
  statusPakan1 = Firebase.getBool("jadwal_pakan/pkn1/status");
  statusPakan2 = Firebase.getBool("jadwal_pakan/pkn2/status");
  if((jamPakan1==hourCurrent && menitPakan1<=minuteCurrent && statusPakan1==0 && readTinggiPakan>pakanMin)){
    Serial.println("pakan 1");
    myservo.write(buka);
    delay(800);
    myservo.write(tutup);
    Firebase.set("control/servo/time", timeRead);
    Firebase.set("control/servo/date", dateRead);
    Firebase.set("jadwal_pakan/pkn1/status", true);
    Firebase.set("control/servo/value", false);
  }
  else if((jamPakan2==hourCurrent && menitPakan2<=minuteCurrent && statusPakan2==0 && readTinggiPakan>pakanMin)){
    Serial.println("pakan 2");
    myservo.write(buka);
    delay(800);
    myservo.write(tutup);
    Firebase.set("jadwal_pakan/pkn2/status", true);
    Firebase.set("control/servo/time", timeRead);
    Firebase.set("control/servo/date", dateRead);
    Firebase.set("control/servo/value", false);
  }
}

void kendaliSuhu(){
  readSuhu = Firebase.getFloat("sensor/suhu");
  suhuMaksimum = Firebase.getFloat("konfigurasiKontrol/suhu_maks");
  suhuMinimum = Firebase.getFloat("konfigurasiKontrol/suhu_min");
  if(readSuhu>=suhuMaksimum){
    digitalWrite(pinHeater, relayBool(0)); 
    digitalWrite(pinCooler, relayBool(1));
    digitalWrite(pinPumpCooler, relayBool(1));
    if(Firebase.getBool("control/cooler/value")==0){
      Firebase.set("control/heater/value", false);
      Firebase.set("control/cooler/value", true);
      Firebase.set("control/cooler/time", timeRead);
      Firebase.set("control/cooler/date", dateRead);
    }
  }
  else if(readSuhu<=suhuMinimum){
    digitalWrite(pinCooler, relayBool(0));
    digitalWrite(pinPumpCooler, relayBool(0));
    digitalWrite(pinHeater, relayBool(1)); 
    if(Firebase.getBool("control/heater/value")==0){
      Firebase.set("control/cooler/value", false);
      Firebase.set("control/heater/value", true);
      Firebase.set("control/heater/time", timeRead);
      Firebase.set("control/heater/date", dateRead);
    }
  }
  else{
    digitalWrite(pinCooler, relayBool(0));
    digitalWrite(pinHeater, relayBool(0));
    Firebase.set("control/cooler/value", false);
    Firebase.set("control/heater/value", false);
  }
}

void kendaliAir(){
  readAmonia = Firebase.getFloat("sensor/amonia");
  readPH = Firebase.getFloat("sensor/ph");
  readTinggiAir = Firebase.getFloat("sensor/tinggi_air");
  amoniaMaks = Firebase.getFloat("konfigurasiKontrol/amonia_maks");
  phMaks = Firebase.getFloat("konfigurasiKontrol/ph_maks");
  phMin = Firebase.getFloat("konfigurasiKontrol/ph_min");
  airMaks = Firebase.getFloat("konfigurasiKontrol/air_maks");
  airMin = Firebase.getFloat("konfigurasiKontrol/air_min");
  if(readAmonia>=amoniaMaks && (readPH<=phMin || readPH>=phMaks) && readTinggiAir>=airMin)
  {
    if(Firebase.getBool("control/drain/value")==0){
      Firebase.set("control/pump/value", false);
      Firebase.set("control/drain/value", true);
      Firebase.set("control/drain/time", timeRead);
      Firebase.set("control/drain/date", dateRead);
    }
    while(readTinggiAir>=airMin){
      digitalWrite(pinPump, relayBool(0));
      digitalWrite(pinDrain, relayBool(1));
      if(modeAlat==0){break;}
      airMin = Firebase.getFloat("konfigurasiKontrol/air_min");
      readTinggiAir = Firebase.getFloat("sensor/tinggi_air");
    }
    Firebase.set("control/drain/value", false);
    digitalWrite(pinPump, relayBool(0));
    digitalWrite(pinDrain, relayBool(0));
  }
  else if(readTinggiAir<=airMaks){
    if(Firebase.getBool("control/pump/value")==0){
      Firebase.set("control/drain/value", false);
      Firebase.set("control/pump/value", true);
      Firebase.set("control/pump/time", timeRead);
      Firebase.set("control/pump/date", dateRead);
    }
    while(readTinggiAir<=airMaks){
      digitalWrite(pinDrain, relayBool(0));
      digitalWrite(pinPump, relayBool(1));
      if(modeAlat==0){break;}
      airMaks = Firebase.getFloat("konfigurasiKontrol/air_maks");
      readTinggiAir = Firebase.getFloat("sensor/tinggi_air");
    }
    Firebase.set("control/pump/value", false);
    digitalWrite(pinPump, relayBool(0));
    digitalWrite(pinDrain, relayBool(0));
  }
  else{
    digitalWrite(pinDrain, relayBool(0));
    digitalWrite(pinPump, relayBool(0));
    Firebase.set("control/pump/value", false);
    Firebase.set("control/drain/value", false);
  }
}

void filter(){
  readKekeruhan = Firebase.getFloat("sensor/kekeruhan");
  keruhMaks = Firebase.getFloat("konfigurasiKontrol/keruh_maks");
  if(readKekeruhan>=keruhMaks){
    digitalWrite(pinUV, relayBool(1));
    if(Firebase.getBool("control/uv/value")==0){
      Firebase.set("control/uv/value", true);
      Firebase.set("control/uv/time", timeRead);
      Firebase.set("control/uv/date", dateRead);
    }
  }
//  else if(hourCurrent>=8 && hourCurrent<17){
//    digitalWrite(pinUV, relayBool(1));
//    if(Firebase.getBool("control/uv/value")==0){
//      Firebase.set("control/uv/value", true);
//      Firebase.set("control/uv/time", timeRead);
//      Firebase.set("control/uv/date", dateRead);
//    }
//  }
  else{
    digitalWrite(pinUV, relayBool(0));
    Firebase.set("control/uv/value", false);
  }
}

bool relayBool(bool value){
  if(value==0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}
