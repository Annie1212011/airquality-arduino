/*
  Reading CO2, humidity and temperature from the SCD30
  By: Nathan Seidle
  SparkFun Electronics
  Date: May 22nd, 2018
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  Feel like supporting open source hardware?
  Buy a board from SparkFun! https://www.sparkfun.com/products/15112

  This example prints the current CO2 level, relative humidity, and temperature in C.

  Hardware Connections:
  Attach RedBoard to computer using a USB cable.
  Connect SCD30 to RedBoard using Qwiic cable.
  Open Serial Monitor at 115200 baud.

  Note: All settings (interval, altitude, etc) are saved to non-volatile memory and are
  loaded by the SCD30 at power on. There's no damage in sending that at each power on.

  Note: 100kHz I2C is fine, but according to the datasheet 400kHz I2C is not supported by the SCD30

  MODIFIED BY NGENS LAB, RTOLEDO-CROW, 11.17.2023
  Used to calibrate the SCD30 referenced to other sensor (e.g. LI-850) or reference gas.
*/

#include <Wire.h>

#include "SparkFun_SCD30_Arduino_Library.h"  //Click here to get the library: http://librarymanager/All#SparkFun_SCD30
SCD30 airSensor;

void setup() {
  Serial.begin(9600);
  delay(8000);
  Serial.println(__FILE__);
  Serial.println("SCD30 settings... ");
  Wire.begin();

  if (airSensor.begin() == false) {
    Serial.println("Air sensor not detected. Please check wiring.");
    while (1) {};
  }

  airSensor.setMeasurementInterval(25);  //Change number of seconds between measurements: 2 to 1800 (30 minutes)

  //  unsigned int altitude = airSensor.getAltitudeCompensation();
  Serial.print("Current altitude compensation: ");
  Serial.print(airSensor.getAltitudeCompensation());
  Serial.println("m");
  airSensor.setAltitudeCompensation(0);  //Set altitude of the sensor in m, stored in non-volatile memory of SCD30

  // set this if you know it. otherwise leave at 0
  airSensor.setAmbientPressure(0);  //Current ambient pressure in mBar: 700 to 1200, will overwrite altitude compensation

  Serial.print("Current temp offset: ");
  Serial.print(airSensor.getTemperatureOffset());
  Serial.println(" C");
  airSensor.setTemperatureOffset(0);

  Serial.print("Self recalibration: ");
  Serial.println(airSensor.getAutoSelfCalibration());
  airSensor.setAutoSelfCalibration(false);

  // un-comment to set CO2 value in chamber. re-comment when done and upload
  //airSensor.setForcedRecalibrationFactor(486);  
}

void loop() {
  if (airSensor.dataAvailable()) {
    Serial.print("\nco2(ppm):");
    Serial.print(airSensor.getCO2());

    Serial.print(" temp(C):");
    Serial.print(airSensor.getTemperature(), 1);

    Serial.print(" humidity(%):");
    Serial.print(airSensor.getHumidity(), 1);

    Serial.println();
  } else
    Serial.print(".");

  delay(1000);
}

