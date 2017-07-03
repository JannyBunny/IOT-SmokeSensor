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

// serielle esp-01 schnittstelle instantiieren
SoftwareSerial espSerial = SoftwareSerial(RXPin, TXPin);

// at-kommando cmd an esp senden, auf antwort ans warten, timeout tout
bool espAtCommand(char *cmd, char *ans, unsigned long tout) 
{
  // timeout fuer find() setzen
  espSerial.setTimeout(tout);
  // kommando senden
  espSerial.print(cmd);
  // nach endestring ans suchen
  if(espSerial.find(ans)) {
    return true;    
  }
  else {    
    return false;
  }
}

// mit wlan verbinden
bool espConnect()
{
  // verbindung zum esp pruefen
  if( !espAtCommand("AT\r\n", "OK", 10000))
    return false;
  // verbindung zum wlan pruefen
  espSerial.print("AT+CWJAP?\r\n");
  if(espSerial.find("+CWJAP:\"")) {
    String wlan = espSerial.readStringUntil('\"');
    // bereits verbunden
    if(wlan == SSID)
      return true;  
  }
  // sonst zum wlan verbinden
  String cmd = "AT+CWJAP=\"";
  cmd += SSID;
  cmd += "\",\"";
  cmd += PASS;
  cmd += "\"\r\n"; 
  if( !espAtCommand(cmd.c_str(), "OK", 15000))
    return false;
  return true;
}

// webserver starten
bool espWebserver(unsigned long tout)
{ 
  int n;
   
  // auf station modus einstellen

  n = 0;
  if(espAtCommand("AT+CWMODE?\r\n", "+CWMODE", 500))
    n = espSerial.parseInt();
  if(n != 1 && !espAtCommand("AT+CWMODE=1\r\n", "OK", 500))
    return false;
  // mehrere verbindungen zulassen
  n = 0;
  if(espAtCommand("AT+CIPMUX?\r\n", "+CIPMUX:", 500))
    n = espSerial.parseInt();
  if(n != 1 && !espAtCommand("AT+CIPMUX=1\r\n", "OK", 500))
    return false;
  if( !espAtCommand("AT+CIPSERVER=1,80\r\n", "OK", 500))
    return false; 
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
  if(espSerial.find("STAIP,\"")) {
    // ip adresse auslesen und liefern
    ipa = espSerial.readStringUntil('\"');
  }
  return ipa;
}

// setup
void setup()
{
  unsigned long t0, t1;
  bool con;
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
  if(con) {
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
  if(ipa.length() > 0) {
    Serial.print("### ip adresse:  ");
    Serial.println(ipa);
  }
  else {
    Serial.println("### could NOT get ip address");
    Serial.println("### program haltet!");
    while(1)
      ;  
  }
  // als webserver auf port 80 starten
  t0 = millis();
  con = espWebserver(1000);
  t1 = millis();
  if(con) {
    Serial.print("### webserver started after ");
    Serial.print(t1 - t0);
    Serial.println(" ms\n");
  }
  else {
    Serial.print("### webserver NOT started in ");
    Serial.print(t1 - t0);
    Serial.println(" ms");
    Serial.println("### program haltet!");
    while(1)
      ;  
  } 
}

// aufruf zaehler
int counter = 0;

// led status
int state = LOW;

// webseite
String webpage1 = "<html>\
<head>\
<title>esp simple webserver</title>\
</head>\
<body bgcolor=#cccccc text=#990000 link=#9990033 vlink=#990033>\
<h2>ESP-01 - Simple Webserver</h2>\
<p>Aufrufe: ";
// hier wird der zaehler eingefuegt
String webpage2 = "</p>\
<p>\
<a href=\"";
// hier wird der link eingefuegt
String webpage3 = "\">";
// hier wird der linktext eingefuegt
String webpage4 = "</a>\
</p>\
</body>\
</html>";

// main loop
void loop()
{
  unsigned long t0, t1;
  int cid, led;
  bool ok1, ok2;
  
  // auf verbindung von client warten
  if(espSerial.available() && espSerial.find("+IPD,")) {
    ++counter;
    Serial.println(">>> connection from client");
    // verbindungsnummer lesen
    cid = espSerial.parseInt();
    // led ggfs. schalten: /led/1 = an, /led/0 = aus
    if(espSerial.find("/led/")) {
      led = espSerial.parseInt();
      if(led == 1)
  state = HIGH;
      else if(led == 0)
  state = LOW;
    }   
    digitalWrite(LEDPin, state);
    // webseite zusammensetzen
    String page = webpage1;
    page += counter;
    page += webpage2; 
    page += state == HIGH ? "/led/0" : "/led/1";
    page += webpage3;
    page += state == HIGH ? "Led Off" : "Led On";
    page += webpage4;
    // laengenkommando: at+cipsend=connection,laenge
    String len = "AT+CIPSEND=";
    len += cid;
    len += ",";
    len += page.length();
    len += "\r\n";
    t0 = millis();
    // laenge senden und auf prompt '>' warten
    espSerial.print(len);
    espSerial.find(">");
    // webseite senden
    page += "\r\n"; 
    espSerial.print(page);
    // senden war ok?
    ok1 = espSerial.find("SEND OK");
    t1 = millis();
    // verbindung schliessen
    String clo = "AT+CIPCLOSE=";
    clo += cid;
    clo += "\r\n";
    espSerial.print(clo);
    // schliessen ok?
    ok2 = espSerial.find("OK");
    // status ausgeben
    if(ok1 && ok2) {    
      Serial.print(">>> page #");
      Serial.print(counter);
      Serial.print(" sent in ");
      Serial.print(t1 - t0);
      Serial.println(" ms");
    }
    else {
      Serial.print(">>> page #");
      Serial.print(counter);
      Serial.print(" NOT sent in ");
      Serial.print(t1 - t0);
      Serial.print(" ms >>> send ");  
      Serial.print(ok1 ? "ok" : "failed");
      Serial.print(" close ");
      Serial.println(ok2 ? "ok" : "failed");
    }
  }
}
