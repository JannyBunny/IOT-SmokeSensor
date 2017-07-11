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
    Sensor:
   -join wifi
   -get/set controller ip
   -create client
      (-scanning local subnet for master ( xyz port) transmit "hellomaster")
        (-calculating Subnet)
      -sending data
        /HelloServer/ Sensor ID (SID=int),analogSensor(analogSensor=int),Alarm(alarm=bool) eg. /HelloServer/1/234/0


   WIFI STUFF
   by PK http://c.rz.hs-fulda.de/ueb09/c09-02-server.ino
*/

// wlan credentials
#define mySSID "IOTwifi"
#define myPASS "iotpk2017"

//server/clientport
#define PORT 80

//connect to host
#define HOST server
IPAddress server = IPAddress(192, 168, 88, 253);

#define SID 1 //SensorID

#define RETRYCOUNTER 10 //loops;  10000+ wenn im Betrieb.

// server objekt
//WiFiServer server(PORT);

WiFiClient client;

// zum wifi verbinden
bool espConnectWifi()
{
  WiFi.begin(mySSID, myPASS);
  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Wifi Connected!");
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
#define SMOKEA0 A0 // Sensor Anschluss
#define SENSORTHRES 400 //400ppm
bool wificonnected = false;
bool alarm = false;
bool didaresend=false;

#define RESENDDATATIMEINMILLIS 10 //10s

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(SMOKEA0, INPUT);
  Serial.begin(BAUD);

  //WIFI
  unsigned long t0, t1;
  // verbinde esp mit dem wlan
  Serial.println("### start esp simple web server\n");
  Serial.println("### connect to wifi"+(String)mySSID);
  t0 = millis();
  wificonnected = espConnectWifi();
  t1 = millis();
  if (wificonnected) {
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
  for (int i = 0; i < 5; i++) {
    if (client.connect(HOST, PORT)) {
      Serial.println("");
      return true;
    }
    Serial.print(".");
    delay(100);
  }
  Serial.println("Could not connect to: "+HOST);
  return false;
}

bool resendDataToServer() {
    //programmlaufzeit aktualisieren
    if(millis()%RESENDDATATIMEINMILLIS==0)
      return true;
    return false;
}

bool checkAlarm(int SensorReading, int alarmValue) {
  // Checks if it has reached the threshold value
  if (SensorReading < alarmValue)
  {
    return false;
  }
  else
  {
    return true;
  }
}

void loop() {

  //Smoke detect
   int analogSensor = analogRead(SMOKEA0);
    Serial.print("Sensor Analog SMOKEA0: ");
    Serial.println(analogSensor);
   alarm=checkAlarm(analogSensor, SENSORTHRES);
   if (analogSensor < SENSORTHRES) 
  {
    digitalWrite(redLed, LOW);
    digitalWrite(greenLed, HIGH);
    digitalWrite(buzzer, LOW);
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
  if (!wificonnected) {
    digitalWrite(redLed, HIGH);
    delay(100); //was 100ms
  }
  if (!wificonnected) { // maybe && (analogSensor < sensorThres) but green goes off anyway.
    digitalWrite(redLed, LOW);
  }
  delay(100); //was 1000ms

  // Just do that, if WIFI is connected;
  if  (wificonnected) {

      unsigned long t0, t1;
  
      if ( !espConnectServer()) {
        Serial.println(">>> connection failed. :( retry");
          if (loops>= RETRYCOUNTER) {
            espConnectServer();
            // zum server verbinden
            Serial.println("RETRY>>> connect to server");
          }
       } //nicht connected    

      
      if( (espConnectServer() && resendDataToServer() ) || alarm ) {  //alle 10s, wenn kein alarm ist
            // zum server verbinden
            Serial.println(">>> connect to server");
            didaresend=true;
            Serial.println(">>> send Hello  ");
            t0=millis();
            String HelloServer="GET [HelloServer/"+(String)SID+"/"+analogSensor+"/"+alarm+" HTTP/1.1\r\nHost: ";
            HelloServer+=HOST.toString().c_str(); // We Have to get an correct IP here, bc IPAdress is an array (:
            HelloServer+="]\r\n";
            
            client.print(HelloServer);
            Serial.println(HelloServer);
    //        Serial.println(client.read());
            delay(5);
    //        Serial.println("Es kommt: "client.peek());
            if(client.peek() < 0) {
                Serial.println("\n\nServer is not talking to me :( Closing...");
                client.print("Connection: close\r\n\r\n");
                client.flush();
                Serial.println("...done");
        
            } 
            else {
            
              client.find('[');
              String antwort=client.readStringUntil(']');
              t1=millis();
              Serial.print("Server Answered for Hello: ");
              Serial.print(antwort);
              Serial.print(" in ");
              Serial.println(t1-t0 +"ms");
              
  
              if (antwort.equals("HelloClient/"+(String)SID+"/"+analogSensor+"/"+alarm)) {
                  String message="GET [OK/"+(String)SID+" HTTP/1.1\r\nHost: ";
                         message+=HOST.toString().c_str(); // We Have to get an correct IP here, bc IPAdress is an array (:
                         message+="]\r\n";
                  t0=millis();
                  client.print(message);
                  Serial.println(message);
                  Serial.println("Data OK ");
                  Serial.println(t1-t0 +"ms");
                }
              else {
                    Serial.println("Echo NOT OK!!");
                    String message="GET [NOK/"+(String)SID+" HTTP/1.1\r\nHost: ";
                           message+=HOST.toString().c_str(); // We Have to get an correct IP here, bc IPAdress is an array (:
                           message+="]\r\n";
                   t0=millis();
                   client.print(message);
                   Serial.println(message);
                   Serial.println("Data NOT OK!");
                   Serial.println(t1-t0 +"ms");
             
                }
                client.flush();
              }
              
      }
        
    
//    String url = "/HelloServer"; 
//    String body;
//    String cl;
//    String  toCollector = "POST ";
//            toCollector += url+ " HTTP/1.1\r\nHost: ";
//            toCollector += HOST;
//            toCollector += "Content-Type: application/x-www-form-urlencoded\r\nContent-Length: "+cl+"\r\n\r\n";
//            body ="SensorID=" + SID;
//            body +="&value=" + analogSensor;
//            body +="&alarm=" + alarm;
//            body +="\r\n";
//            toCollector += body;
//    cl=body.length();

//    client.print(toCollector);

    
  }
  
  //Retry WIFI if not Connected
  if ((loops > RETRYCOUNTER) && !wificonnected) {
    Serial.println("## Retry Wificonnection");
    wificonnected = espConnectWifi();
    loops = 0;
  }//for wificonnectretry
  
  Serial.println(">>> wait a while - 1s ");
  delay(1000);
  loops++;
}
