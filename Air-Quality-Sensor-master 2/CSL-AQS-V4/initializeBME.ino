// Adafruit BME280 T P RH sensor

void initializeBME()  {
  Serial.println("BME280 not used");
  display.println("BME280 not used");
  display.display();
}

String readBME()  {
  return "0.00, 0.00, 0.00, ";
}
