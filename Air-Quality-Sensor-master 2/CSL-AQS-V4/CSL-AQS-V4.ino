
/*
   COMMUNITY SENSOR LAB - AIR QUALITY SENSOR

   featherM0-Wifi + featherwing adalogger-SD-RTC + SCD30-CO2 + BME280-TPRH + OLED display + SPS30-PM2.5

   The SCD30 has a minimum power consumption of 5mA and cannot be stop-started. It's set to 55s (30s nominal)
   sampling period and the featherM0 sleeps for 2 x 16 =32s, wakes and waits for data available.
   Button A toggles display on/off but must be held down for 16s max and then wait 16s to toggle again.

   Logs: date time, co2, t, rh, t2, press, rh2, battery voltage, status

   https://github.com/Community-Sensor-Lab/Air-Quality-Sensor

   Global status is in uint8_t stat in bit order:
   0- 0000 0001 0x01 SD card not present
   1- 0000 0010 0x02 SD could not create file
   2- 0000 0100 0x04 RTC failed
   3- 0000 1000 0x08 SCD30 CO2 sensor not available
   4- 0001 0000 0x10 SCD30 CO2 sensor timeout
   5- 0010 0000 0x20 SPS30 PM2.5 sensor malfunction
   6- 0100 0000 0x40 HSC differential pressure sensor absent or malfunction
   7- 1000 0000 0x80 future: google.com ssl connection error

   RICARDO TOLEDO-CROW NGENS, ESI, ASRC, CUNY,
   AMALIA TORRES, CUNY, July 2021

*/
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_SleepyDog.h>
#include "RTClib.h"
#include "SparkFun_SCD30_Arduino_Library.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>  // oled library
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SensirionI2CSen5x.h>
#include <WiFi101.h>
#include <HoneywellTruStabilitySPI.h>  // for differential pressure sensor for Met https://github.com/huilab/HoneywellTruStabilitySPI.git
#include <FlashStorage.h>


#define VBATPIN A7  // this is also D9 button A disable pullup to read analog
#define BUTTON_A 9  // Oled button also A7 enable pullup to read button
#define BUTTON_B 6  // oled button
#define BUTTON_C 5  // oled button
#define SD_CS 10    // Chip select for SD card default for Adalogger
// #define HSC_CS 12   // Chip select for Honeywell HSC diff press sensor
#define MAXBUF_REQUIREMENT 48

#if (defined(I2C_BUFFER_LENGTH) &&            \
(I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
(defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

typedef struct {
  boolean valid;
  char saved_ssid[64];
  char saved_passcode[64];
  char saved_gsid[128];
} Secrets;

FlashStorage(flash_storage, Secrets);


char server[] = "harbor-airquality.netlify.app";  // name address for Google scripts as we are communicationg with the scripg (using DNS)
// these are the commands to be sent to the google script: namely add a row to last in Sheet1 with the values TBD
String payload_base = "{\"command\":\"appendRow\",\"sheet_name\":\"Sheet1\",\"values\":";
String payload = "";
char header[] = "DateTime, CO2, Tco2, RHco2, Tbme, Pbme, RHbme, vbat(mV), status, mP1.0, mP2.5, mP4.0, mP10, ncP0.5, ncP1.0, ncP2.5, ncP4.0, ncP10, avgPartSize, Thsc, dPhsc";
int status = WL_IDLE_STATUS;
String ssidg, passcodeg, gsidg;
uint16_t CO2;  // for oled display
float Pmv = 0;
float Nox = 0;
float Voc = 0;
bool force_pro = false;

SensirionI2CSen5x sen5x;
WiFiSSLClient client;                                            // make SSL client
RTC_PCF8523 rtc;                                                 // Real Time Clock for RevB Adafruit logger shield
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);       // large OLED display
Adafruit_BME280 bme;                                             // the bme tprh sensor
File logfile;                                                    // the logging file
SCD30 CO2sensor;                                                 // sensirion SCD30 CO2 NDIR
// TruStabilityPressureSensor diffPresSens(HSC_CS, -100.0, 100.0);  // HSC differential pressure sensor for Met Eric Breunitg
uint8_t stat = 0;                                                // status byte

void setup(void) {
  pinMode(VBATPIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(9600);
  delay(5000);
  Serial.println(__FILE__);
  WiFi.setPins(8, 7, 4, 2);

  initializeOLED();
  initializeSen5x();         // PM sensor
  initializeSCD30(25);       // CO2 sensor to 30s more stable (1 min max recommended)
  initializeBME();           // TPRH
  logfile = initializeSD();  // SD card and RTC
  delay(3000);

  //Set Interrupt
  pinMode(BUTTON_A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_A), A, CHANGE);


  if (!check_valid()) {
    AP_getInfo(ssidg, passcodeg, gsidg);
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connecting to previously saved network");
    display.display();
    storeinfo(ssidg, passcodeg, gsidg);
  }
}

//Interrupt Handler
void A() {  
  force_pro = true;
}

char outstr[160];
int32_t Tsleep = 0;
bool displayState = true;
bool buttonAstate = true;
int lastTimeToggle = 0;
int timeDebounce = 100;

void loop(void) {

  uint8_t ctr = 0;
  stat = stat & 0xEF;  // clear bit 4 for CO2 sensor

  String bmeString = readBME();  // get data string from BME280 "T, P, RH, "
  String bme = readBME();

  // parsing out the t p rh float values
  float Tbme = bme.toFloat();
  bme = bme.substring(bme.indexOf(", ") + 2);
  float Pbme = bme.toFloat();
  bme = bme.substring(bme.indexOf(", ") + 2);
  float RHbme = bme.toFloat();


  String sen5xString = readSen5x();
  String sen5x = readBME();

  String co2String = readSCD30(Pbme);

  DateTime now;
  now = rtc.now();  // fetch the date + time

  pinMode(VBATPIN, INPUT);  // read battery voltage
  float measuredvbat = analogRead(VBATPIN) * 0.006445;
  pinMode(BUTTON_A, INPUT_PULLUP);

  delay(5000);  // wait for the sps30 to stabilize

  //  sprintf(outstr, "%02u/%02u/%02u %02u:%02u:%02u, %.2d, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %x, ",
  //          now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(),
  //          CO2, Tco2, RHco2, Tbme, Pbme, RHbme, measuredvbat, stat);

  sprintf(outstr, "%02u/%02u/%02u %02u:%02u:%02u, ", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  payloadUpload(String(outstr) + co2String + bmeString + String(measuredvbat) + String(", ") + String(stat) + String(", ") + sen5xString);

  Serial.println(header);
  Serial.println(String(outstr) + co2String + bmeString + String(measuredvbat) + String(", ") + String(stat) + String(", ") + sen5xString);

  logfile.println(String(outstr) + co2String + bmeString + String(measuredvbat) + String(", ") + String(stat) + String(", ") + sen5xString);
  logfile.flush();  // Write to disk. Uses 2048 bytes of I/O to SD card, power and takes time

  // sleep cycle
  for (int i = 1; i <= 8; i++) {  // 124s = 8x16s sleep, only toggle display
    displayState = toggleButton(BUTTON_A, displayState, buttonAstate, lastTimeToggle, timeDebounce);
    if (displayState) {  // turn display on with data
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("CO2 ppm ");
      display.print(CO2);
      display.print("V ");
      display.println(measuredvbat);
      display.print("T C ");
      display.println(Tbme);
      display.print("P mBar ");
      display.println(Pbme);
      display.print("RH% ");
      display.println(RHbme);
      display.print("VOC ");
      display.println(Voc);
      display.print("NOX ");
      display.println(Nox);
      display.print("NewPM ");
      display.print(Pmv);
      display.display();
    } else {  // turn display off
      display.clearDisplay();
      display.display();
    };
    //int sleepMS = Watchdog.sleep();// remove comment for low power
    delay(16000);  // uncomment to debug because serial communication doesn't come back after sleeping
  }
}
