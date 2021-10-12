/**
 * @file ThermistorCalc.ino
 * 
 * @brief This file calculates the resistance of an NTC thermistor based on output voltage.  
 *        Now contains code to trasnmit a message to another RFM69HCW radio module
 * @author Mark Maurer (mmark2)
 * @date 10/6/2021
 */
#include <math.h> //for pow and log functions
#include <RH_RF69.h> //RadioHead library for radio functions
#include <SPI.h> 
#include <String.h> //for string functions and converting char array to string

#define RF69_FREQ   433.0   //setting frequency to 433 MHz

//The following three lines refer to the arduino pin which
//handles the various radio inputs
#define RFM69_RST     2     //reset pin
#define RFM69_INT     3     //interrupt pin
#define RFM69_CS      4     //chis select pin

RH_RF69 rf69(RFM69_CS, RFM69_INT);  // radio driver object

//the coefficients for the Steinhart-Hart equation
//calculated using matrix in matlab
float A = 9.191896699*pow(10, -3);
float B = -10.68933374*pow(10, -4);
float C = 51.48508572*pow(10, -7);

void setup() {
  Serial.begin(1000000); //start serial monitor
  pinMode(A0, INPUT); //set A0 to an input to read the output voltage from the voltage divider
  pinMode(RFM69_RST, OUTPUT); //configuring the reset pin
  digitalWrite(RFM69_RST, LOW); //set it low initially
  // manual reset, setting the RST pin high triggers reset of module
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  //check to make sure the reset worked
  if (!rf69.init()) 
  {
    Serial.println("RFM69 radio init failed");
    while (1); //will enter an infinite loop if initialization fails
  }
  else
  {
    Serial.println("RFM69 radio init OK!");
  }
  rf69.setFrequency(RF69_FREQ); //set the frequency
  rf69.setModemConfig(20);       //set the modulation configuration, always produces warning
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW
}

void loop() 
{
  delay(2000);
  double Vout = analogRead(A0) * (5/1023.0);  //analog read gives a number between 1-1023
                                              //so output voltage is calculated by
                                              //multiplying by source voltage and
                                              //dividing by 1023.
  double rTHR = (10000 * Vout) / (5 - Vout);  //equation of resistance derived from voltage divider equation
  float temp; //the temperature variable
  temp = 1 / (A + (B*log(rTHR)) + (C*pow(log(rTHR), 3))); //the Steinhart-hart equation to find temp
  temp -= 273.15; //convert to celcius
  temp *= 9;
  temp /= 5;
  temp += 32; //converted to fahrenheit, the steps were expanded to avoid rounding errors
  Serial.print("My temp is :");
  Serial.println(temp);

  //place to store incoming data
  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  //look for a message from the transmitter radio
  if (rf69.waitAvailableTimeout(500)) 
  {
    if (rf69.recv(buf, &len)) 
    {
      if (!len) return;
      buf[len] = 0;   //null terminate the buffer
      Serial.print("Received temp is: ");
      Serial.println((char*)buf);       //cast the buffer as a character array
      String myStr = (char*)buf;        //create a string for the data
      float recTemp = myStr.toFloat();  //convert the string to a float value
      float avg = (temp + recTemp) / 2; //compute the average
      Serial.print("The average temp is: ");
      Serial.println(avg, 2);
      Serial.print("RSSI: "); //RSSI = Receiver Signal Strength Indicator
      Serial.println(rf69.lastRssi(), DEC); //this function returns the most recent indicator for the last message received
      // Send a reply so that the transmitter knows the message was received
      //uint8_t data[] = "Got the temp";
      //rf69.send(data, sizeof(data));
      //rf69.waitPacketSent();
      //Serial.println("Sent a reply");
    } 
  }
}
