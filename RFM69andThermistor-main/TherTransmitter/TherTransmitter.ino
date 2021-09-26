/**
 * @file ThermistorCalc.ino
 * 
 * @brief This file calculates the resistance of an NTC thermistor based on output voltage.  
 *        Now contains code to trasnmit a message to another RFM69HCW radio module
 * @author Mark Maurer (mmark2)
 * @date 9/25/2021
 */
#include <math.h> //for pow and log functions
#include <RH_RF69.h> //RadioHead library for radio functions
#include <SPI.h>
#include <String.h> //to convert char array to strings and back again, also has useful functions

//these three lines not used yet
#define NETWORKID     100 // Must be the same for all nodes
#define MYNODEID      1   // My node ID
#define TONODEID      2   // Destination node ID

#define RF69_FREQ   433.0   //setting frequency to 433 MHz

#if defined (__AVR_ATmega328P__)  // UNO or Feather 328P w/wing
  #define RFM69_INT     3  // 
  #define RFM69_CS      4  //
  #define RFM69_RST     2  // "A"
  #define LED           13 //not used yet
#endif

//radio driver object
RH_RF69 rf69(RFM69_CS, RFM69_INT);

//the coefficients for the Steinhart-Hart equation
//calculated using matrix in matlab
double A = 9.191896699*pow(10, -3);
double B = -10.68933374*pow(10, -4);
double C = 51.48508572*pow(10, -7);

void setup() {
  Serial.begin(9600); //start serial monitor
  pinMode(A0, INPUT); //set A0 to an input to read 
                      //the output voltage from the voltage divider

  while (!Serial) 
  { 
    delay(1); 
  } // wait until serial console is open, remove if not tethered to computer

  pinMode(LED, OUTPUT);     //not used yet, will be used to indicate data
                            //being received or transmitted
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

  //setting the modulation configuration
  //Rb = 250 kbs and Fd = 250 kHz
  rf69.setFrequency(RH_RF69::GFSK_Rb250Fd250);
  
  if (!rf69.setFrequency(RF69_FREQ)) 
  {
    Serial.println("setFrequency failed");
  }
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW
}

void loop() {
  
  delay(1000); //wait for a second
  double Vout = analogRead(A0) * (5/1023.0);  //analog read gives a number between 1-1023
                                              //so output voltage is calculated by
                                              //multiplying by source voltage and
                                              //dividing by 1023.
  double rTHR = (10000 * Vout) / (5 - Vout);  //equation of resistance derived from voltage divider equation
  
  double temp; //the temperature variable

  //use the Steinhart-Hart equation to calculate temp based on current resistance
  //value, calculated in kelvins
  temp = 1 / (A + (B*log(rTHR)) + (C*pow(log(rTHR), 3)));
  temp -= 273.15; //convert to celcius
  temp *= 9;
  temp /= 5;
  temp += 32; //converted to fahrenheit, the steps were expanded to avoid rounding errors

  //get ready to transmit
  delay(1000);  // Wait 1 second between transmits
  
  Serial.print("Sending "); 
  Serial.println(temp, 0); //essentially prints temp as an int
  
  rf69.waitPacketSent(); //wait for it to be sent, blocks the radio until done

  // Now wait for a reply
  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN]; //this is where a received message is stored
                                        //make it the max length possible
  uint8_t len = sizeof(buf);            //this is the length or size of the message
  
  //start converting the temp integer to a character array
  //this code will use itoa() function described below
  //itoa(value to convert, array to store the null-terminated string, base number(I'll use 10))
  itoa(temp, (char*)buf, 10);
  
  rf69.send((uint8_t *)buf, len); //calls the send function, sends an unsignedint or char
                                  //the second argument is the byte size of the message
  //check for a message
  //starts the receiver and blocks until a received message is
  //available or a timeout occurs
  if (rf69.waitAvailableTimeout(500))  //might need to adjust the wait time on the transmitter 
  { 
    //call the recv function, turns on the receiver if it isn't on
    //copies the message to buf, then resets the value of len to the
    //length of the message
    if (rf69.recv(buf, &len)) {
      //Serial.print("Got a reply: ");
      //Serial.println((char*)buf);
    } else {
      Serial.println("Receive failed");
    }
  } else {
    Serial.println("Waiting...");
  }
}
