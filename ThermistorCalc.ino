/**
 * @file ThermistorCalc.ino
 * 
 * @brief This file calculates the resistance of an NTC thermistor based on output voltage.  
 * 
 * @author Mark Maurer (mmark2)
 * @date 9/19/2021
 */
#include <math.h>
#include <stdio.h>

//declaring global calculating averages
const int runs = 10;    //number of runs
int i = 0;              //counter

float temps[runs];

//the coefficients for the Steinhart-Hart equation
//calculated using matrix in matlab
float A = 9.191896699*pow(10, -3);
float B = -10.68933374*pow(10, -4);
float C = 5.148508572*pow(10, -6);

void setup() {
  Serial.begin(9600); //start serial monitor
  pinMode(A0, INPUT); //set A0 to an input to read 
                      //the output voltage from the voltage divider
}

void loop() {
  
  double Vout = analogRead(A0) * (5/1023.0);  //analog read gives a number between 1-1023
                                              //so output voltage is calculated by
                                              //multiplying by source voltage and
                                              //dividing by 1023.
  double rTHR = (10000 * Vout) / (5 - Vout);  //equation of resistance derived from voltage divider equation
  
  float temp; //the temperature variable
  
  
  if (i < runs)
  {
    Serial.println();
    Serial.print("Recording number: ");
    Serial.println(i);
    Serial.print("Current output voltage Vout = "); //displays the current output voltage of the divider
    Serial.println(Vout);
    Serial.print("Resistance of thermistor is "); //displays the current resistance
    Serial.print(rTHR);
    Serial.println(" ohms");
    //displays temp
    temp = 1 / (A + (B*log(rTHR)) + (C*pow(log(rTHR), 3)));//calculate temp in K
    temp -= 273.15; //convert to celcius
    temp *= 9;
    temp /= 5;
    temp += 32; //converted to fahrenheit, the steps were expanded to avoid rounding errors
    temps[i] = temp;
    Serial.print("Temperature is ");
    Serial.print(temp, 3);
    Serial.print(" degrees fahrenheit");
    Serial.println();
    i++;  //increase the counter
  }
  else 
  {
    //Print the temperature after ten recordings
    Serial.println();
    Serial.println("End of trial result");
    float sum = 0;  //sum up all the recordings
    for(int i = 0; i < runs; i++)
    {
      sum += temps[i];
    }
    float avgTemp = sum / runs; // take the average
    Serial.print("Average Temperature is ");
    Serial.print(avgTemp, 3);
    Serial.println(" degrees fahrenheit.");
    Serial.end();
  }
  delay(3000);//delays the serial monitor output by 3000 ms or 3 seconds
              //just for readability
}
