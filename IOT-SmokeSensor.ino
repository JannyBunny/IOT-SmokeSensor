#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

#include <ESP8266HTTPClient.h>
/*******
 
 All the resources for this project:
 https://www.hackster.io/Aritro

*******/

/*
 *  Sensor:
   -join wifi
   -get/set controller ip
   -create client
      (-scanning local subnet for master ( xyz port) transmit "hellomaster")
        (-calculating Subnet)
      -sending data
        clientid(cid=int),CurrentValue(cval=int),Alarm(alarm=bool) eg. 1,234,false
 * 
 * 
 * WIFI STUFF
 * by PK http://c.rz.hs-fulda.de/ueb09/c09-02-server.ino
 */
 
// wlan credentials
#define mySSID "IOTwifi"
#define myPASS "iotpk2017"

//server/clientport
#define PORT 80

//connect to host
#define HOST server
IPAddress server(192,168,88,252);

#define SID 1 //SensorID

#define RETRYCOUNTER 10 //loops;  10000+ wenn im Betrieb.

// server objekt
//WiFiServer server(PORT);

WiFiClient client;

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

int loops = 0;
int redLed = D5;
int greenLed = D2;
int buzzer = D1;
int smokeA0 = A0; 
int sensorThres = 400;// 400ppm
bool wificonnected = false;
bool alarm=false;

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(smokeA0, INPUT);
  Serial.begin(BAUD);

//WIFI
 unsigned long t0, t1;
  // verbinde esp mit dem wlan
  Serial.println("### start esp simple web server\n");
  Serial.println("### connect to wifi");
  t0 = millis();
  wificonnected = espConnectWifi();
  t1 = millis();
  if(wificonnected) {
    Serial.print("### connected after ");
    Serial.print(t1 - t0);
    Serial.println(" ms\n");
  }
  else {
    Serial.print("### NOT connected in ");
    Serial.print(t1 - t0);
    Serial.println(" ms");
    Serial.println("### Aborting: connect WIFI, please!");
  }
  // Server starten und ip adresse ausgeben
//  server.begin();
//  Serial.print("### server started. ip adresse:  ");
  Serial.println(WiFi.localIP());
  
} //Setup

// zum server verbinden
bool espConnectServer() {
  for(int i = 0; i < 5; i++) {
    if(client.connect(HOST, PORT)) {
        Serial.println("");
        return true;
        }
     Serial.print(".");
     delay(100);
     }
  return false; 
}
         
//// webseite
//String webpage1 = "<html>\
//<head>\
//<title>esp simple webserver</title>\
//</head>\
//<body bgcolor=#cccccc text=#990000 link=#9990033 vlink=#990033>\
//<h2>ESP8266 - Simple Webserver</h2>\
//<p>GAS: ";
//// hier wird die temperatur eingefuegt
//String webpage2 = "</p></body></html>";

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
    alarm=false;
  }
  else
  {
    digitalWrite(redLed, HIGH);
    digitalWrite(greenLed, LOW);
    tone(buzzer, 1000, 200); //digitalWrite(buzzer, HIGH);
    Serial.print("ALARM!");
    alarm=true;
  }

  //WIFI not Connected => LED Blinks
  if (!wificonnected){
    digitalWrite(redLed, HIGH);
     delay(1000); //was 100ms
  }
  if (!wificonnected){ // maybe && (analogSensor < sensorThres) but green goes off anyway.
    digitalWrite(redLed, LOW);
  }
  delay(100); //was 1000ms

 // Just do that, if WIFI is connected;
 if  (wificonnected) {
    
      unsigned long t0, t1;
      
//      // auf verbindung von client warten
//      WiFiClient client = server.available();
//      if( !client)
//        return;
//      // request von client lesen
//      String req = client.readStringUntil('\r');
//      Serial.print(">>> client request: ");
//      Serial.println(req);
//      client.flush();
//      // webseite zusammensetzen
//      String page = webpage1;
//      page += analogSensor;
//      page += webpage2; 
//      page += "\r\n"; 
//      // seite an client senden
//      t0 = millis();
//      client.print(page);
//      t1 = millis();
//      delay(5);
//      Serial.print(">>> page ");
//      Serial.print(" sent in ");
//      Serial.print(t1 - t0);
//      Serial.println(" ms");


  // zum server verbinden
  Serial.println(">>> connect to server");
  if( !espConnectServer()) {
    Serial.println(">>> connection failed. :( retry in "+RETRYCOUNTER);
    
  }
  Serial.print(">>> send request, analogSensor: ");
  Serial.println(analogSensor);
//  String req = "GET ";
//  req += PAGE;
//  req += analogSensor;
//  req += " HTTP/1.1\r\n";
//  req += "Host: ";
//  req += HOST;
//  req += "\r\n";
//  req += "Connection: close\r\n\r\n";
//  client.print(req);
  String url = "/";
       
  Serial.print("Requesting URL: ");
  Serial.println(url);
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                       "Host: " + HOST + "\r\n" + 
                       "Content-Type: application/x-www-form-urlencoded\r\n" + 
                       "Content-Length: 13\r\n\r\n" +
                       "SensorID"= + SID +
                       "value=" + analogSensor + 
                       "alarm=" + alarm +
                       "\r\n");

  // antwort vom server lesen und ausgeben
  Serial.print(">>> read answer: ");
  String ans = client.readString();
  Serial.println(ans);
  // warten bis zum naechsten zyklus
  Serial.print(">>> wait a while - ");
  delay(100);
  }
  
  //Retry WIFI if not Connected
  if ((loops > RETRYCOUNTER) && !wificonnected) {
      Serial.println("## Retry Wificonnection");
      wificonnected = espConnectWifi();
      loops = 0;
  }
  
  //for wificonnectretry
  loops++;
}
