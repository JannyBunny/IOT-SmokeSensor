/*******
 
 All the resources for this project:
 https://www.hackster.io/Aritro

*******/

int redLed = D5;
int greenLed = D2;
int buzzer = D1;
int smokeA0 = A0;
// Your threshold value
int sensorThres = 300;

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(smokeA0, INPUT);
  Serial.begin(115200); //was 9600
}

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
  delay(1000); //was 100ms
}
