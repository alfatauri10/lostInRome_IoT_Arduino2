/*
  Questo sketch legge i dati dei sensori e li scrive su un Database Firebase real-time via WIFI  
  sensore di temperatura collegato a A1
  sensore di umidità del terreno collegato a A0
  
 */

#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <DHT.h>


bool wifiConnect();
bool dbPost(String pathJson, String jsonBody);
bool dbPut(String pathJson, String jsonBody);
void readSensors();


// ====== IMPOSTAZIONI WIFI ======
//TODO: da modificare con i dati del WIFI che avremo al maker faire
#define WIFI_SSID "Galilei TEST"
#define WIFI_PASSWORD "Coniglio21"

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


 delay(10000); // 10 secondi tra un invio e l'altro
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
//const String&
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
 
  //temperatureC  = 20.0 + (random(0, 50) / 10.0); //VALORE CABLATO PER TEST
  temperatureC = ((((analogRead(A1)*5.0) / 1024.0) - 0.5) * 100);

  //soil_moisture = 2000 + random(0, 100); //VALORE CABLATO PER TEST
  //soil_moisture = ((500/10.23)-100)*(-1);
  soil_moisture = ((analogRead(SOILANALOGPIN)/10.23)-100)*(-1);



  Serial.println("Letture sensori:");
  Serial.print("  T: "); Serial.println(temperatureC);
  Serial.print("  Soil perc: "); Serial.println(soil_moisture);
}

void readSensorsOffset() {
  // Lettura del sensore con la formula standard
  temperatureC = ((((analogRead(A1) * 5.0) / 1024.0) - 0.5) * 100);

  // Aggiungi un offset di calibrazione per correggere l'errore
  float offset = 0; // Esempio: sottrai 20 gradi
  temperatureC = temperatureC + offset;

  Serial.println("Letture sensori:");
  Serial.print("  T: ");
  Serial.println(temperatureC);
  Serial.print("  Soil perc: "); Serial.println(soil_moisture);
}
