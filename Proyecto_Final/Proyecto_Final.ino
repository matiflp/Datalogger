#include <Time.h>

const unsigned char ledPin = 7;
const unsigned char sensorTemp = A0;
const unsigned char trigPin = 9;
const unsigned char echoPin = 8;
const unsigned int  start = 0xFFFE;
const unsigned char stopt = 0xFE;
const unsigned long DEFAULT_TIME = 1560240000; //Fecha y hora en formato Unix
int encendido = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
  pinMode(ledPin, OUTPUT);  // Sets the ledPin as an Output
  setTime(DEFAULT_TIME);
}

void loop()
{
  
 if (Serial.available())
 {
   encendido = Serial.parseInt();
 
   if (encendido == 1)
   {
   digitalWrite(ledPin,HIGH);
   }else 
   {
   digitalWrite(ledPin,LOW);
   }
 }
  
  if (encendido == 1)
  {
  //Distancia
  //Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  long pulse = pulseIn(echoPin, HIGH);
 
  // Temperatura
  int value = analogRead(sensorTemp);
  
  //Comienzo de trama con dos bytes
  Serial.write( highByte( start));
  Serial.write( lowByte( start));
  
  //Hora
  digitalClockDisplay();  
  
  //Temperatura
  Serial.write( highByte(value));
  Serial.write( lowByte(value));
  
  //Distancia
  byte a,b,c,d;
  a |= pulse;
  pulse = pulse >> 8;
  b |= pulse;
  pulse = pulse >> 8;
  c |= pulse;
  pulse = pulse >> 8;
  d |= pulse;
  pulse = pulse >> 8;
  
  Serial.write(d);//Parte alta
  Serial.write(c);
  Serial.write(b);
  Serial.write(a);//Parte baja
  
  //Byte de fin de trama
  Serial.write(stopt);
  }
}

void digitalClockDisplay(){
  byte hours,minutes,seconds,days,months;
  int years;
  // digital clock display of the time
  
  hours = hour();
  Serial.write(hours);

  minutes = minute();
  Serial.write(minutes); 

  seconds = second();
  Serial.write(seconds);
  
  days = day();
  Serial.write(days);
  
  months = month();
  Serial.write(months);

  years = year();
  Serial.write( highByte(years));
  Serial.write( lowByte(years));
}

