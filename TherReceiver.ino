/**
 * @file ThermistorCalc.ino
 * 
 * @brief This file calculates the resistance of an NTC thermistor based on output voltage.  
 *        Now contains code to trasnmit a message to another RFM69HCW radio module
 * @author Mark Maurer (mmark2)
 * @date 9/23/2021
 */
#include <math.h>
#include <RH_RF69.h>
#include <SPI.h>

#define NETWORKID     100 // Must be the same for all nodes
#define MYNODEID      1   // My node ID
#define TONODEID      2   // Destination node ID

#define RF69_FREQ   433   //setting frequency to 433 MHz

#if defined (__AVR_ATmega328P__)  // UNO or Feather 328P w/wing
  #define RFM69_INT     3  // 
  #define RFM69_CS      4  //
  #define RFM69_RST     2  // "A"
  #define LED           13 //not used yet
#endif

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

//declaring global calculating averages
const int runs = 10;    //number of runs

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
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  //use setModemConfig(ModemConfigChoice index) to change modulation setting
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) 
  {
    Serial.println("setFrequency failed");
  }
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
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

  delay(1000);  
  int itemp = temp; //convert the temp value to an int

  //place to store incoming data
  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  //look for a message from the transmitter radio
  if (rf69.available()) {
    if (rf69.recv(buf, &len)) {
      if (!len) return;
      buf[len] = 0;
      Serial.print("Received [");
      Serial.print(len);
      Serial.print("]: ");
      Serial.println((char*)buf);

      int recTemp = buf;
      //RSSI = Receiver Signal Strength Indicator
      Serial.print("RSSI: ");
      //this function returns the most recent indicator for the last message received
      Serial.println(rf69.lastRssi(), DEC);
      
      // Send a reply so that the transmitter knows the message was received
      uint8_t data[] = "Got the temp";
      rf69.send(data, sizeof(data));
      rf69.waitPacketSent();
      Serial.println("Sent a reply");
    } 
    else 
    {
      Serial.println("Receive failed");
    }
  }
}
