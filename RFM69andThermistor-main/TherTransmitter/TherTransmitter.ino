/**
 * @file ThermistorCalc.ino
 * 
 * @brief This file calculates the resistance of an NTC thermistor based on output voltage.  
 *        Now contains code to trasnmit a message to another RFM69HCW radio module
 * @author Mark Maurer (mmark2)
 * @date 9/25/2021
 */
#include <math.h> //for pow and log functions
#include <RFM69.h>
#include <SPI.h> 
#include <String.h> //for string functions and converting char array to string

//next three lines aren't used yet
#define NETWORKID     0 // Must be the same for all nodes
#define MYNODEID      2   // My node ID
#define TONODEID      1   // Destination node ID

#define FREQUENCY   RF69_433MHZ   //setting frequency to 433 MHz
#define ENCRYPT       false // Set to "true" to use encryption
#define ENCRYPTKEY    "TOPSECRETPASSWRD" // Use the same 16-byte key on all nodes

// Use ACKnowledge when sending messages (or not):
#define USEACK        false // Request ACKs or not

// radio driver, this is the object which uses the radio
RFM69 radio;

//the coefficients for the Steinhart-Hart equation
//calculated using matrix in matlab
float A = 9.191896699*pow(10, -3);
float B = -10.68933374*pow(10, -4);
float C = 51.48508572*pow(10, -7);

void setup() {
  Serial.begin(9600); //start serial monitor
  pinMode(A0, INPUT); //set A0 to an input to read 
                      //the output voltage from the voltage divider

  radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
  radio.setHighPower(); //required for RFM69HCW module

  if(ENCRYPT)
  {
    radio.encrypt(ENCRYPTKEY);
  }
}

void loop() {
  double Vout = analogRead(A0) * (5/1023.0);  //analog read gives a number between 1-1023
                                              //so output voltage is calculated by
                                              //multiplying by source voltage and
                                              //dividing by 1023.
  double rTHR = (10000 * Vout) / (5 - Vout);  //equation of resistance derived from voltage divider equation
  
  float temp; //the temperature variable

  //use the Steinhart-Hart equation to calculate temp based on current resistance
  //value, calculated in kelvins
  temp = 1 / (A + (B*log(rTHR)) + (C*pow(log(rTHR), 3)));
  temp -= 273.15; //convert to celcius
  temp *= 9;
  temp /= 5;
  temp += 32; //converted to fahrenheit, the steps were expanded to avoid rounding errors

  delay(500);  

  //place to store incoming data
  static char dataBuffer[62];
  static int dataLength = 6;
  // SENDING
  // Send the packet!
  sprintf(dataBuffer, "%d", temp);

  Serial.print("sending to node ");
  Serial.print(TONODEID, DEC);
  Serial.print(": [");
  for (byte i = 0; i < dataLength; i++)
    Serial.print(dataBuffer[i]);
  Serial.println("]");
  
  // There are two ways to send packets. If you want
  // acknowledgements, use sendWithRetry():
  
  if (USEACK)
  {
    if (radio.sendWithRetry(TONODEID, dataBuffer, dataLength))
      Serial.println("ACK received!");
    else
      Serial.println("no ACK received :(");
  }

  // If you don't need acknowledgements, just use send():
  
  else // don't use ACK
  {
    radio.send(TONODEID, dataBuffer, dataLength);
  }
  
  dataLength = 0; // reset the packet
}
