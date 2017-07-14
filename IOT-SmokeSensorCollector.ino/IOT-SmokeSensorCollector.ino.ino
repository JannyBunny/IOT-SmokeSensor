//#include "RedkeaWiFi.h"
//
//char ssid[] = "IOTwifi";
//char pass[] = "iotpk2017";
//char deviceID[] = "-KockqvTbvZYYgXmgVg1";
//
//RedkeaWiFi redkea;
//
///**************************************************************************************************
//Use the following snippet if you want to send data from your device to a widget.
//Replace "mySenderFunction" with the name of the user function you chose when you created the widget.
//(Don't change "widgetID".)
//***************************************************************************************************/
//
//
//
//REDKEA_SENDER(getValue, widgetID)
//{
//    // use the following lines to send a string to a text widget
//    
//     String str = "Test"; // assign the string you want to display 
//     redkea.sendToTextWidget(widgetID, RKgetValue()); //str
//}
//REDKEA_REGISTER_SENDER(redkea, getValue)
//
//
//
///**************************************************************************************************
//Use the following snippet if you want to receive data from a widget.
//Replace "myReceiverFunction" with the name of the user function you chose when you created the widget.
//(Don't change "args.")
//***************************************************************************************************/
//
//
//
//REDKEA_RECEIVER(getInfo, args)
//{
//  // use this line to receive a value from a toggle widget
//     bool toggleState = redkea.readFromToggleWidget(args);
//  
//  
//    // use this line to receive a value from a touch widget
//    // bool touchState = redkea.readFromTouchWidget(args);
//
//
//    // use this line to receive a value from a slider widget
//    // int sliderValue = redkea.readFromSliderWidget(args);
//}
//REDKEA_REGISTER_RECEIVER(redkea, getInfo)

//String RKgetValue() {
//  if (messwerte[mess_len][2]==1){
//    return "ALARM!";  
//  }
//  else
//    return "Alles OK";
//}



//Inspiriert von:
// demo06-esp-webserver.ino
// klin, 30.05.2017
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
    Serial.println("Found answer: " + (String)ans+" From ESP");
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


//Hau Webseite mit allen Werten (SensorID,AnalogInput,Alarm) raus
String printWebsite(int SensorID,int AnalogInput,int Alarm) {
    String website="<html><head><title></title></head><body><h1>Sensor Reading</h1><p> SensorID: | AnalogInput:  | Alarm?</p><p>";
    website+=SensorID;
    website+="            | ";
    website+=AnalogInput;
    website+="            | ";
    if (Alarm) {
      website+="ALARM!!!";
    }
    else {
      website+="OK";
    }
    website+="</p></body></html>";
    return website;
}

//länge der Daten
#define DATA_ARRAYSIZE 3
//messwerte 3x1 (SID),(AnalogRead),(True/False[Alarm],(time) memorymemorymemory :(
//String messwerte[3][1];
int messwerte[DATA_ARRAYSIZE][3];
//"länge" des arrays, wo er vorher wieder anfängt neu reinzuschreiben
int mess_len=0;


// setup
void setup()
{ 
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

//  //redkea stuff
//  redkea.begin(ssid, pass, deviceID);
  
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
  String AnswerHS,AnswerData;
  for (int i=0;i<mess_len;i++) {
      //Serial.println(i);
      AnswerData+=printWebsite(messwerte[i][0],messwerte[i][1],messwerte[i][2]); 
      
    }
  // auf verbindung von client warten
    //Serial.println(espSerial.peek() );
    if (espSerial.available() && espSerial.find("+IPD,")) {
        cid=espSerial.parseInt();
        if (espSerial.find("[HelloServer")){
            Serial.println("Found HelloServer");
            sid=espSerial.parseInt();
            espSerial.find("/");
            analog=espSerial.parseInt();
            espSerial.find("/");
            alarm=espSerial.parseInt();
                    AnswerHS="[[HelloClient/";
                    AnswerHS+=sid;
                    AnswerHS+="/";
                    AnswerHS+=analog;
                    AnswerHS+="/";
                    AnswerHS+=alarm;
                    AnswerHS+="]]";
            String HelloC ="AT+CIPSEND=";
            HelloC += cid;
            HelloC += ",";
            HelloC += AnswerHS.length();
            HelloC += "\r\n";
            t0 = millis();
            espSerial.print(HelloC);
            Serial.println("send HelloClient"+AnswerHS);
            
             // laenge senden und auf prompt '>' warten
            espSerial.find("]]");

            //Antwort senden
            AnswerHS += "\r\n"; //Antwort senden
            espSerial.print(AnswerHS);
            delay(5);
            
            
            if (espSerial.find("SEND OK")) {
                Serial.println("Send HelloClient OK!");
                t1 = millis();
            }
         
            t2=millis();
            //Serial.println(espSerial.readString());
            if (espSerial.find("[OK/")) {
                  sid=espSerial.parseInt();
                  Serial.println("SENSOR DATA READ "+(String)sid+" OK!");
                  if (mess_len>=DATA_ARRAYSIZE){
                      mess_len=0;
                      Serial.println("Array is full. beginning at 0");
                      
                  }
                  messwerte[mess_len][0]=sid;
                  messwerte[mess_len][1]=analog;
                  messwerte[mess_len][2]=alarm;
                  //messwerte[mess_len][3]=; // reserved for timestamp
                  Serial.println("wrote to array");
                  mess_len++;
                  Serial.println("Written Data to array");
                }//ifok
                else if(espSerial.find("[NOK/"))
                  {
                    sid=espSerial.parseInt();
                    Serial.println("SENSOR DATA READ NOT OK!" +(String)cid+" SensorID:" +(String)sid);
                  }//elseNOK
                } //HelloServer

               else  
                if (espSerial.find("GET /DATA/")) {
                  Serial.println("Sending /DATA/ " );
                  
                  String laenge="AT+CIPSEND=";
                  laenge += cid;
                  laenge += ",";
                  laenge += AnswerData.length();
                  laenge += "\r\n";
                  t0 = millis();
                  espSerial.print(laenge);
                  
                  // laenge senden und auf prompt '>' warten
                  espSerial.find(">");
            
                  //Antwort senden
                  AnswerData += "\r\n"; //Antwort senden
                  espSerial.print(AnswerData);
                
                  if (espSerial.find("SEND OK")) {
                      Serial.println("Send Website OK!");
                      t1 = millis();
                  }
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
                }//Data
              //}//else 

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
      } //if ipd
     
  //Serial.println("Messwerte AlarmStatus: "+messwerte[mess_len-1][2]); 
  if (alarm ) { //alarm state of the last run otherwise, try alarm && messwerte[mess_len-1][2]
    state= HIGH;
  }
  digitalWrite(LEDPin, state);
  digitalWrite(10, HIGH);//DEBUG LED GELB
  loops++;
  //espSerial.flush();
  //delay(500);
  //Serial.flush(); //BAD BAD

//  //redkeaLoop
//  redkea.loop();
}
