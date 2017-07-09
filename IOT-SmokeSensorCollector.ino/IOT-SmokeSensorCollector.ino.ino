#include "RedkeaWiFi.h"

char ssid[] = "SSID";
char pass[] = "PASS";
char deviceID[] = "-KockqvTbvZYYgXmgVg1";

RedkeaWiFi redkea;

/**************************************************************************************************
Use the following snippet if you want to send data from your device to a widget.
Replace "mySenderFunction" with the name of the user function you chose when you created the widget.
(Don't change "widgetID".)
***************************************************************************************************/



REDKEA_SENDER(getValue, widgetID)
{
    // use the following lines to send a string to a text widget
    
     String str = "asdf"; // assign the string you want to display 
     redkea.sendToTextWidget(widgetID, str);
}
REDKEA_REGISTER_SENDER(redkea, getValue)

REDKEA_SENDER(getAnalog, widgetID)
{
    // use the following lines to send a string to a text widget
    
     String str = "blah"; // assign the string you want to display 
     redkea.sendToTextWidget(widgetID, str);
}
REDKEA_REGISTER_SENDER(redkea, getAnalog)



/**************************************************************************************************
Use the following snippet if you want to receive data from a widget.
Replace "myReceiverFunction" with the name of the user function you chose when you created the widget.
(Don't change "args.")
***************************************************************************************************/

/*

REDKEA_RECEIVER(myReceiverFunction, args)
{
  // use this line to receive a value from a toggle widget
    // bool toggleState = redkea.readFromToggleWidget(args);
  
  
    // use this line to receive a value from a touch widget
    // bool touchState = redkea.readFromTouchWidget(args);


    // use this line to receive a value from a slider widget
    // int sliderValue = redkea.readFromSliderWidget(args);
}
REDKEA_REGISTER_RECEIVER(redkea, myReceiverFunction)

*/



// demo06-esp-webserver.ino
// klin, 30.05.2017
//
// der sketch realisiert einen einfachen webserver
// mit aufrufzaehler und link zum schalten einer led
//
// kommunikation mit esp-01 ueber software serial
// das esp-01-modul ist ueber den esp-adapter angeschlossen
//
// der sketch funktioniert fuer die at-firmware
// ai-thinker at-version 1.2.0.0 und sdk-version 1.5.4.1
// ai-thinker at-version 1.1.0.0 und sdk-version 1.5.4
// ai-thinker at-version 0.40.0.0 und sdk-version 1.3.0.2
// ai-thinker at-version 0.25.0.0 und sdk-version 1.1.1
// ai-thinker at-version 0.21.0.0 und sdk-version 0.9.5
// andere firmware-versionen muessen evtl. angepasst werden
//
// arduino - esp-01-adapter
//      5v - vcc
//     gnd - gnd
//   rxpin - txd
//   dxpin - rxd

/*
 * to FIX Serial reset: 100µF Capacitor between RESET and Ground or Cut the solderbridge "REST ON" ( no:( )
 */
/*
   2do:
   Controller:
   -join wifi
   -create server on port
      -listening on port xyz for
          clientid(cid=int),CurrentValue(cval=int),Alarm(alarm=bool)
          (answering "helloclient" for call "hellomaster")

    Daten speichern

    redkea app einbinden
    datenexportfunktion für Redkea APP
    
    Sensor daten senden erst alle 10sek
    
    modularisierung

  

*/

// wlan credentials
#define SSID "IOTwifi"
#define PASS "iotpk2017"

// esp-01 ueber software serial
#include <SoftwareSerial.h>

// pretzelboard mit esp-01 on-board: rxpin=11, txpin=12, baud=19200
// rxpin und dxpin
#define RXPin 2
#define TXPin 3

// baudrate serial monitor und esp-01
#define BAUD 9600

// led pin
#define LEDPin 9

//für debug der Verbindung
#define DEBUGLOOP 10000

//TCP Port
#define PORT 80

// serielle esp-01 schnittstelle instantiieren
SoftwareSerial espSerial = SoftwareSerial(RXPin, TXPin);

// at-kommando cmd an esp senden, auf antwort ans warten, timeout tout
bool espAtCommand(char *cmd, char *ans, unsigned long tout)
{
  // timeout fuer find() setzen
  espSerial.setTimeout(tout);
  // kommando senden
  Serial.println("Sending command to ESP: " + (String)cmd);
  espSerial.print(cmd);
  // nach endestring ans suchen
  if (espSerial.find(ans)) {
    Serial.println("Found answer: " + (String)ans);
    return true;
  }
  else {
    Serial.println("No answer from ESP :( ");
    return false;
  }
}

// mit wlan verbinden
bool espConnect()
{
  // verbindung zum esp pruefen
  if ( !espAtCommand("AT\r\n", "OK", 10000)) {
    Serial.println("Got no AT - OK from ESP");
    return false;
  }
  // verbindung zum wlan pruefen
  espSerial.print("AT+CWJAP?\r\n");
  if (espSerial.find("+CWJAP:\"")) {
    String wlan = espSerial.readStringUntil('\"');
    // bereits verbunden
    if (wlan == SSID)
      Serial.println("Already connected to: " + wlan);
    return true;
  }
  // sonst zum wlan verbinden
  Serial.println("Trying to Connect...");
  Serial.println("Listing wifi...");

  String cmd = "AT+CWJAP=";
  cmd += "\"";
  cmd += SSID;
  cmd += "\",\"";
  cmd += PASS;
  cmd += "\"\r\n";

  if ( !espAtCommand(cmd.c_str(), "OK", 15000))
    return false;
  return true;
}

// webserver starten
bool espWebserver(unsigned long tout)
{
  int n;

  // auf station modus einstellen

  n = 0;
  if (espAtCommand("AT+CWMODE?\r\n", "+CWMODE", 500)) {
    Serial.println("Check CWMode");
    n = espSerial.parseInt();
  }
  if (n != 1 && !espAtCommand("AT+CWMODE=1\r\n", "OK", 500)) {
    Serial.println("Fehler: CWMode= " + n);
    return false;
  }
  // mehrere verbindungen zulassen
  n = 0;
  if (espAtCommand("AT+CIPMUX?\r\n", "+CIPMUX:", 500)) {
    n = espSerial.parseInt();
    Serial.println("cipmux herausbekommen");
  }
  if (n != 1 && !espAtCommand("AT+CIPMUX=1\r\n", "OK", 500)) {
    Serial.println("n=1&!cipmux1");
    return false;
  }
  if ( !espAtCommand("AT+CIPSERVER=1,80\r\n", "OK", 500)) {
    Serial.println("Server Starten auf port :");
    Serial.print(PORT);
    return false;
  }
  return true;
}

// ip-adresse lesen und ausgeben
String espIPAddress(unsigned long tout)
{
  String ipa = "";
  espSerial.flush();
  espSerial.setTimeout(tout);
  // kommando senden
  espSerial.print("AT+CIFSR\r\n");
  if (espSerial.find("STAIP,\"")) {
    // ip adresse auslesen und liefern
    ipa = espSerial.readStringUntil('\"');
  }
  return ipa;
}


//messwerte 3x1 (SID),(AnalogRead),(True/False[Alarm],(time) memorymemorymemory :(
//String messwerte[3][1];
String messwerte;

// setup
void setup()
{
    //REDKEA STUFF
    redkea.begin(ssid, pass, deviceID);
  
  digitalWrite(10, LOW);//DEBUG LED GELB
  unsigned long t0, t1;
  bool con,webserver;
  // setze baudrate fuer serial und esp-01 sowie led modus
  Serial.begin(BAUD);
  espSerial.begin(BAUD);
  pinMode(LEDPin, OUTPUT);
  // verbinde esp mit dem wlan
  Serial.println("### start esp simple web server\n");
  Serial.println("### connect to wifi");
  t0 = millis();
  con = espConnect();
  t1 = millis();
  if (con) {
    Serial.print("### connected after ");
    Serial.print(t1 - t0);
    Serial.println(" ms\n");
  }
  else {
    Serial.print("### NOT connected in ");
    Serial.print(t1 - t0);
    Serial.println(" ms");
  }
  // ip adresse lesen
  String ipa = espIPAddress(2000);
  if (ipa.length() > 0) {
    Serial.print("### ip adresse:  ");
    Serial.println(ipa);
  }
  else {
    Serial.println("### could NOT get ip address");
    //    Serial.println("### program haltet!");
    //    while(1)
    //      ;
  }
  if(!webserver) {
    webserver = espWebserver( t0);
    return true;
  }
  Serial.println("End of Setup");
}

// aufruf zaehler
int counter,loops = 0;

// led status
int state = LOW;

// main loop
void loop()
{
  unsigned long t0, t1,t2,t3;
  int cid, led, sid, analog;
  bool ok1, ok2, alarm;
  
  
  // auf verbindung von client warten
    if (espSerial.available() && espSerial.find("+IPD,")) {
//  if (espSerial.available()  ) {

        cid=espSerial.parseInt();
        Serial.println("client id="+cid);
//      String conn= espSerial.readString();
//      Serial.println(conn);
        if (espSerial.find("/HelloServer")){
            Serial.println("Found HelloServer");
            sid=espSerial.parseInt();
            espSerial.find("/");
            analog=espSerial.parseInt();
            espSerial.find("/");
            alarm=espSerial.parseInt();
            Serial.println(sid+analog+alarm);
            String AnswerHS="[HelloClient/";
                    AnswerHS+=sid;
                    AnswerHS+="/";
                    AnswerHS+=analog;
                    AnswerHS+="/";
                    AnswerHS+=alarm;
                    AnswerHS+="]";
            String HelloC ="AT+CIPSEND=";
            HelloC += cid;
            HelloC += ",";
            HelloC += AnswerHS.length();
            HelloC += "\r\n";
            t0 = millis();
            espSerial.print(HelloC);
            Serial.println("send HelloClient"+AnswerHS);
            
             // laenge senden und auf prompt '>' warten
            espSerial.find(">");
      
            //Antwort senden
            AnswerHS += "\r\n"; //Antwort senden
            espSerial.print(AnswerHS);
          
            if (espSerial.find("SEND OK")) {
                Serial.println("Send HelloClient OK!");
                t1 = millis();
            }
         
          t2=millis();
          if (espSerial.readString().equals("/OK/")) {
              if (cid==espSerial.parseInt()) {
                  Serial.println("SENSOR DATA READ OK!");
                  messwerte=analog;
                }
                else if(espSerial.readString().equals("/NOK/")) {
                  if (cid==espSerial.parseInt()) {
                    Serial.println("SENSOR DATA READ NOT OK!" +cid);
                  }
                }
          }
          String data = espSerial.readString();
          t3=millis();
          Serial.print("Data: ");
          Serial.println(data);
          Serial.println(" read in: ");
          Serial.print(t3-t2);
          
          Serial.println("Sending data back");
          t2=millis();
          espSerial.print(data);
          t3=millis();
          Serial.print("Data: ");
          Serial.print(" send back in: ");
          Serial.print(t3-t2);
          data+=";"+millis();
          messwerte=data;
          // verbindung schliessen
          String clo = "AT+CIPCLOSE=";
          clo += cid;
          clo += "\r\n";
          espSerial.print(clo);
          Serial.print(">>> page #");
          Serial.print(counter);
          Serial.print(" sent in ");
          Serial.print(t1 - t0);
          Serial.println(" ms");
          espSerial.flush();
      }
      
//    if (espSerial.find("+IPD,HelloServer")) {    
//    Serial.println(">>> connection from client");
//    
//    Serial.println(espSerial.read());
//    
    //}
    
    // verbindungsnummer lesen
//    cid = espSerial.parseInt();
//    // led ggfs. schalten: /led/1 = an, /led/0 = aus
//    if (espSerial.find("/led/")) {
//      led = espSerial.parseInt();
//      if (led == 1)
//        state = HIGH;
//      else if (led == 0)
//        state = LOW;
//    }
//    digitalWrite(LEDPin, state);
//    // webseite zusammensetzen
//    String page = webpage1;
//    page += counter;
//    page += webpage2;
//    page += state == HIGH ? "/led/0" : "/led/1";
//    page += webpage3;
//    page += state == HIGH ? "Led Off" : "Led On";
//    page += webpage4;
//    // laengenkommando: at+cipsend=connection,laenge
//    String len = "AT+CIPSEND=";
//    len += cid;
//    len += ",";
//    len += page.length();
//    len += "\r\n";
//    t0 = millis();
//    // laenge senden und auf prompt '>' warten
//    espSerial.print(len);
//    espSerial.find(">");
//    // webseite senden
//    page += "\r\n";
//    espSerial.print(page);
//    // senden war ok?
//    ok1 = espSerial.find("SEND OK");
//    t1 = millis();
//    // verbindung schliessen
//    String clo = "AT+CIPCLOSE=";
//    clo += cid;
//    clo += "\r\n";
//    espSerial.print(clo);
//    // schliessen ok?
//    ok2 = espSerial.find("OK");
//    // status ausgeben
//    if (ok1 && ok2) {
//      Serial.print(">>> page #");
//      Serial.print(counter);
//      Serial.print(" sent in ");
//      Serial.print(t1 - t0);
//      Serial.println(" ms");
//    }
//    else {
//      Serial.print(">>> page #");
//      Serial.print(counter);
//      Serial.print(" NOT sent in ");
//      Serial.print(t1 - t0);
//      Serial.print(" ms >>> send ");
//      Serial.print(ok1 ? "ok" : "failed");
//      Serial.print(" close ");
//      Serial.println(ok2 ? "ok" : "failed");
//    }
  }
//  if (loops >= DEBUGLOOP){
//    Serial.println("no Connection from client :( ");
//    loops=0;
//  }
  if (alarm) {
    state= HIGH;
  }
  digitalWrite(LEDPin, state);
  digitalWrite(10, HIGH);//DEBUG LED GELB
  loops++;
  //espSerial.flush();
  delay(500);
  //Serial.flush(); //BAD BAD
  
  //redkea Stuff
  redkea.loop();
}
