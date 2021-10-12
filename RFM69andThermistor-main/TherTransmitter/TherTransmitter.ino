/**
 * @file ThermistorCalc.ino
 * 
 * @brief This file calculates the resistance of an NTC thermistor based on output voltage.  
 *        Now contains code to trasnmit a message to another RFM69HCW radio module
 * @author Mark Maurer (mmark2)
 * @date 9/25/2021
 */
#include <math.h> //for pow and log functions
#include <RH_RF69.h>
#include <SPI.h> 
#include <String.h> //for string functions and converting char array to string

#define RF69_FREQ   433.0   //setting frequency to 433 MHz
#define RFM69_INT     3  
#define RFM69_CS      4  
#define RFM69_RST     2 

RH_RF69 rf69(RFM69_CS, RFM69_INT);  //radio driver object


#define FREQUENCY   RF69_433MHZ   //setting frequency to 433 MHz
#define ENCRYPT       false // Set to "true" to use encryption
#define ENCRYPTKEY    "TOPSECRETPASSWRD" // Use the same 16-byte key on all nodes

// Use ACKnowledge when sending messages (or not):
#define USEACK        false // Request ACKs or not


//the coefficients for the Steinhart-Hart equation
//calculated using matrix in matlab
float A = 9.191896699*pow(10, -3);
float B = -10.68933374*pow(10, -4);
float C = 51.48508572*pow(10, -7);

void setup() {
  Serial.begin(1000000); //start serial monitor
  pinMode(A0, INPUT); //set A0 to an input to read 
                      //the output voltage from the voltage divider
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
    while (1);
  }
  else
  {
    Serial.println("RFM69 radio init OK!");
  }
  
  rf69.setFrequency(RF69_FREQ);
  rf69.setModemConfig(20);
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW
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
  
  Serial.print("Sending "); 
  Serial.println(temp, 2); // print the floating value to 2 decimal places
  rf69.waitPacketSent(); //wait for it to be sent, blocks the radio until done
  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN]; //this is where a message to be sent is stored
  uint8_t len = sizeof(buf);            //this is the length or size of the message
  String myStr = "";              //start converting the temp data into a data type that can be sent, create an empty string object
  myStr.concat(temp);             //concatenate the temp data to the end of the string
  myStr.toCharArray(buf, len);    //change the string to character array and store it in the buffer
  rf69.send((uint8_t *)buf, len); //calls the send function, sends an unsignedint or char
                                  //the second argument is the byte size of the message

}
