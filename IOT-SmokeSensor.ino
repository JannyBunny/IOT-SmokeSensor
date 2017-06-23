/*******
 
 All the resources for this project:
 https://www.hackster.io/Aritro

*******/

/*
 * WIFI STUFF
 * by PK http://c.rz.hs-fulda.de/ueb09/c09-02-server.ino
 */
// wlan credentials
#define mySSID "your-ssid"
#define myPASS "your-password"

#define PORT 80

#include <ESP8266WiFi.h>

// server objekt
WiFiServer server(PORT);

// zum wifi verbinden
bool espConnectWifi()
{
  WiFi.begin(mySSID, myPASS);
  for(int i = 0; i < 20; i++) {
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      return true;
    }  
    Serial.print(".");
    delay(500);
  }
  return false;    
}

// baudrate serial monitor
#define BAUD 115200

int redLed = D5;
int greenLed = D2;
int buzzer = D1;
int smokeA0 = A0;
// Your threshold value
int sensorThres = 300;
bool wificonnecterror = false;

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(smokeA0, INPUT);
  Serial.begin(BAUD);

//WIFI
 unsigned long t0, t1;
  bool c;
  // verbinde esp mit dem wlan
  Serial.println("### start esp simple web server\n");
  Serial.println("### connect to wifi");
  t0 = millis();
  c = espConnectWifi();
  t1 = millis();
  if(c) {
    Serial.print("### connected after ");
    Serial.print(t1 - t0);
    Serial.println(" ms\n");
    wificonnecterror=false;
  }
  else {
    Serial.print("### NOT connected in ");
    Serial.print(t1 - t0);
    Serial.println(" ms");
    Serial.println("### aborting WIFI connect please!");
    wificonnecterror=true;
  }
  // Server starten und ip adresse ausgeben
  server.begin();
  Serial.print("### server started. ip adresse:  ");
  Serial.println(WiFi.localIP());
  
}


// webseite
String webpage1 = "<html>\
<head>\
<title>esp simple webserver</title>\
</head>\
<body bgcolor=#cccccc text=#990000 link=#9990033 vlink=#990033>\
<h2>ESP8266 - Simple Webserver</h2>\
<p>GAS: ";
// hier wird die temperatur eingefuegt
String webpage2 = "</p></body></html>";

void loop() {
  int analogSensor = analogRead(smokeA0);

  Serial.print("Pin A0: ");
  Serial.println(analogSensor);

  // Checks if it has reached the threshold value
  if (analogSensor < sensorThres)
  {
    digitalWrite(redLed, LOW);
    digitalWrite(greenLed, HIGH);
    digitalWrite(buzzer,LOW);
  }
  else
  {
    digitalWrite(redLed, HIGH);
    digitalWrite(greenLed, LOW);
    tone(buzzer, 1000, 200); //digitalWrite(buzzer, HIGH);
    Serial.print("ALARM!");
  }
  if (wificonnecterror){
    digitalWrite(redLed,LOW);
  }
  delay(1000); //was 100ms
  if (wificonnecterror){
    digitalWrite(redLed,HIGH);
  }


 unsigned long t0, t1;
  
  // auf verbindung von client warten
  WiFiClient client = server.available();
  if( !client)
    return;
  // request von client lesen
  String req = client.readStringUntil('\r');
  Serial.print(">>> client request: ");
  Serial.println(req);
  client.flush();
  // webseite zusammensetzen
  String page = webpage1;
  page += analogSensor;
  page += webpage2; 
  page += "\r\n"; 
  // seite an client senden
  t0 = millis();
  client.print(page);
  t1 = millis();
  delay(5);
  Serial.print(">>> page ");
  Serial.print(" sent in ");
  Serial.print(t1 - t0);
  Serial.println(" ms");
  
}
