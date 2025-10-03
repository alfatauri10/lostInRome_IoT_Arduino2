/*
  Questo sketch legge i dati dei sensori e li scrive su un Database Firebase real-time via WIFI  
  sensore di temperatura collegato a A1
  sensore di umidità del terreno collegato a A0
  
 */

#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;



bool wifiConnect();
bool dbPost(String pathJson, String jsonBody);
bool dbPut(String pathJson, String jsonBody);
void readSensors();
void readTemp();
void readUmidita();
void stampaValoriSuMonitorSeriale();
void setupMatrix();
void loopMatrix();


// ====== IMPOSTAZIONI WIFI ======
//TODO: da modificare con i dati del WIFI che avremo al maker faire
#define WIFI_SSID "Galilei TEST"
#define WIFI_PASSWORD "Coniglio21"

//#define WIFI_SSID "iPhone di Valentina"
//#define WIFI_PASSWORD "111111111"

//#define WIFI_SSID  "TIM-35780934"
//#define WIFI_PASSWORD "3E5p6TYtDG3Zftk5qeb4HyXe"


#define DATABASE_HOST "lostinrome-sensori-default-rtdb.firebaseio.com"


#define SOILANALOGPIN A0
#define SOILDIGITALPIN 8


// ====== Client HTTPS + HttpClient verso il Realtime Database ======
WiFiSSLClient ssl;
HttpClient httpDB(ssl, DATABASE_HOST, 443);


// ====== Valori sensori ======
float temperatureC   = 0.0;
int   soil_moisture  = 0;


void setup() {
  Serial.begin(115200);

  setupMatrix();

  analogReference(AR_EXTERNAL);
  while (!Serial) { ; }
  pinMode(SOILDIGITALPIN, INPUT);
  pinMode(SOILANALOGPIN, INPUT);


  Serial.println("Connessione WiFi...");
  if (!wifiConnect()) {
    Serial.println("WiFi non connesso.");
    return;
  }
  Serial.print("Connesso. IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Sistema pronto.");
}


void loop() {

 loopMatrix();
 if (WiFi.status() != WL_CONNECTED) {
   Serial.println("WiFi disconnesso. Riconnessione...");
   if (!wifiConnect()) {
     delay(2000);
     return;
   }
 }

 readSensors();


 // Costruisci payload JSON da scrivere sul DB Firebase
 DynamicJsonDocument doc(512);
 doc["temperature"]   = temperatureC;
 doc["soil_moisture"] = soil_moisture;


 String jsonStr;
 serializeJson(doc, jsonStr);


 Serial.println("Payload JSON:");
 Serial.println(jsonStr);



 // ===== OPZIONE B: SOVRASCRIVI (PUT) percorso fisso =====
 bool ok = dbPut("/sensors.json", jsonStr);


 Serial.println(ok ? "Dati inviati!" : "Invio fallito.");


 delay(5000); // 10 secondi tra un invio e l'altro
}




// ------- Helper: connessione WiFi -------
bool wifiConnect() {
 WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 unsigned long start = millis();


 // Attende connessione
 while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
   delay(300);
 }
 if (WiFi.status() != WL_CONNECTED) return false;


 // Attende un IP valido (evita 0.0.0.0)
 IPAddress ip = WiFi.localIP();
 start = millis();
 while ((ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) && millis() - start < 10000) {
   delay(200);
   ip = WiFi.localIP();
 }
 return !(ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0);
}

// ------- Helper: POST JSON su un path del DB -------
bool dbPost(String pathJson, String jsonBody) {
 // Esempio pathJson: "/sensors.json"
 httpDB.beginRequest();
 httpDB.post(pathJson.c_str(), "application/json", jsonBody);
 httpDB.endRequest();
 readSensors();


 int status = httpDB.responseStatusCode();
 String body = httpDB.responseBody();


 Serial.print("[POST] Status: ");
 Serial.println(status);
 if (body.length()) {
   Serial.println("Body:");
   Serial.println(body);
 }
 return (status >= 200 && status < 300);
}


// ------- Helper: PUT JSON (sovrascrive il nodo) -------
bool dbPut(String pathJson, String jsonBody) {
 // Esempio pathJson: "/sensors.json"
 httpDB.beginRequest();
 httpDB.put(pathJson.c_str(), "application/json", jsonBody);
 httpDB.endRequest();


 int status = httpDB.responseStatusCode();
 String body = httpDB.responseBody();


 Serial.print("[PUT] Status: ");
 Serial.println(status);
 if (body.length()) {
   Serial.println("Body:");
   Serial.println(body);
 }
 return (status >= 200 && status < 300);
}


void readSensors() {
  readTemp();
  readUmidita();
  stampaValoriSuMonitorSeriale();
}

void readTemp(){
  int val_Adc = 0;

  //eseguo un ciclo
  for(byte i = 0; i < 100; i++){
    //acquisisco il valore e lo sommo alla variabile
    val_Adc += analogRead(A1);
    //questo ritardo serve per dare il tempo all ADC di eseguire correttamente la prossima acquisizione
    delay(10);
  }
  //eseguo la media dei 100 valori letti
  val_Adc /= 100;


  //calcolo la temperatura in °C
  temperatureC = ((val_Adc * 0.0032) - 0.5) / 0.01; //valore temperatura vicino al reale
  //temperatureC  = 20.0 + (random(0, 50) / 10.0); //VALORE CABLATO PER TEST
  //temperatureC = ((((analogRead(A1)*5.0) / 1024.0) - 0.5) * 100); //non funzionante

}

void readUmidita(){
  //soil_moisture = 2000 + random(0, 100); //VALORE CABLATO PER TEST
  //soil_moisture = ((500/10.23)-100)*(-1);
  soil_moisture = ((analogRead(SOILANALOGPIN)/10.23)-100)*(-1);
}

void stampaValoriSuMonitorSeriale() {
  Serial.println("Letture sensori:");
  Serial.print("  T: "); Serial.println(temperatureC);
  Serial.print("  Soil perc: "); Serial.println(soil_moisture);
}

void setupMatrix(){
  matrix.begin();

  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  // add some static text
  const char text[] = "";
  matrix.textFont(Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText();
  matrix.endDraw();
  delay(2000);
}

void loopMatrix(){
   matrix.beginDraw();

  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(50);

  // add the text
  const char text[] = " LostInRome WIFI";
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText(SCROLL_LEFT);

  matrix.endDraw();
}
