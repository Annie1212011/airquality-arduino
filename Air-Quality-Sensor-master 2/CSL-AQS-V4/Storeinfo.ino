/*
  Store and retrieve structured data in Flash memory.

  This example code is in the public domain.

  Written 30 Apr 2015 by Cristian Maglie
*/

// #include <FlashStorage.h>

// Create a structure that is big enough to contain a saved_ssid
// and a saved_passcode. The "valid" variable is set to "true" once
// the structure is filled with actual data for the first time.
// typedef struct {
//   boolean valid;
//   char saved_ssid[64];
//   char saved_passcode[64];
//   char saved_gsid[128];
// } Secrets;

// Reserve a portion of flash memory to store a "Secrets" and
// call it "flash_storage".
// FlashStorage(flash_storage, Secrets);

// Note: the area of flash memory reserved lost every time
// the sketch is uploaded on the board.

bool check_valid(){
  Secrets info;
  info = flash_storage.read();
  return info.valid;
}


void storeinfo(String &ssid, String &passcode, String &gsid) {
  // SERIAL_PORT_MONITOR.begin(9600);
  // while (!SERIAL_PORT_MONITOR) {}

  // Create a "Secrets" variable and call it "info"
  Secrets info;

  // Read the content of "flash_storage" into the "info" variable
  info = flash_storage.read();

  // If this is the first run the "valid" value should be "false"...
  if (info.valid == false) {

    // ...in this case we ask for user data.
    //SERIAL_PORT_MONITOR.setTimeout(30000);
    //SERIAL_PORT_MONITOR.println("Insert your saved_ssid:");
    String saved_ssid = ssid;
    //SERIAL_PORT_MONITOR.println("Insert your saved_passcode:");
    String saved_passcode = passcode;
    String saved_gsid = gsid;

    // Fill the "info" structure with the data entered by the user...
    saved_ssid.toCharArray(info.saved_ssid, 64);
    saved_passcode.toCharArray(info.saved_passcode, 64);
    saved_gsid.toCharArray(info.saved_gsid, 128);
    // set "valid" to true, so the next time we know that we
    // have valid data inside
    info.valid = true;

    // ...and finally save everything into "flash_storage"
    flash_storage.write(info);

    // Print a confirmation of the data inserted.
    display.clearDisplay();
    display.setCursor(0, 0);
    SERIAL_PORT_MONITOR.println();
    SERIAL_PORT_MONITOR.print("Your saved_ssid: ");
    display.print("Your saved ssid: ");
    SERIAL_PORT_MONITOR.println(info.saved_ssid);
    display.println(info.saved_ssid);
    SERIAL_PORT_MONITOR.print("and your saved_passcode: ");
    display.print("Your saved passcode: ");
    SERIAL_PORT_MONITOR.println(info.saved_passcode);
    display.println(info.saved_passcode);
    SERIAL_PORT_MONITOR.print("along with your saved_gsid: ");
    display.print("Your saved gsid: ");
    SERIAL_PORT_MONITOR.println(info.saved_gsid);
    display.println(info.saved_gsid);
    SERIAL_PORT_MONITOR.println("have been saved.");
    display.println("have been saved.");
    display.display();
    return;

  } else {

    //Allow User to force provisioning if they want to update the wifi information or if they have moved to a different location
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("If you would like to force provisioning, press button a");
    display.display();
    delay(2000);
    int i = 0;
    while (i < 80000){
      Serial.print(" ");
      if (force_pro){
        info.valid = false;
        flash_storage.write(info);
        AP_getInfo(ssid, passcode, gsid);
        return;
      }
      i++;
    }


    //Since the variables in the parameter are passed by reference we are able to modify the global variables value directly
    //As such we can assign the saved values to the global variable that will then be used to connect to the wifi and the google sheet
    ssid = info.saved_ssid;
    passcode = info.saved_passcode;
    gsid = info.saved_gsid;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connecting to previously saved network...");
    display.print("Saved ssid: ");
    display.println(info.saved_ssid);
    display.print("Saved passcode: ");
    display.println(info.saved_passcode);
    display.print("Saved gsid: ");
    display.println(info.saved_gsid);
    display.display();
    SERIAL_PORT_MONITOR.println(info.saved_ssid);
    SERIAL_PORT_MONITOR.println(info.saved_passcode);
    SERIAL_PORT_MONITOR.println(info.saved_gsid);
    delay(5000);
    return;
  }
}
