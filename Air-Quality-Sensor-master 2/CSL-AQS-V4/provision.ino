//WiFiServer server(80);
/* this is the simple webpage with three fields to enter and
send info */

const char webpage_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Community Sensor Lab provisioning page</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    ssid: <input type="text" name="SSID"><br>
    <!-- <input type="submit" value="Submit">
  </form><br>
  <form action="/get"> -->
    passcode: <input type="password" name="passcode"><br>
    <!-- <input type="submit" value="Submit">
   </form><br>
 <form action="/get"> -->
    gsid: <input type="text" name="GSID"><br>
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

/**
* print wifi relevant info to Serial
*
*/

String urlDecode(String input) {
  String decoded = "";
  char temp[] = "0x00";
  
  for (int i = 0; i < input.length(); i++) {
    if (input[i] == '%') {
      temp[2] = input[i+1];
      temp[3] = input[i+2];
      decoded += (char)strtol(temp, NULL, 16);
      i += 2;
    } else if (input[i] == '+') {
      decoded += ' ';
    } else {
      decoded += input[i];
    }
  }
  return decoded;
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print(F("signal strength (RSSI):"));
  Serial.print(rssi);
  Serial.println(F(" dBm"));

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connect to SSID: ");
  display.println(WiFi.SSID());
  display.display();
}

/**
* print formatted MAC address to Serial
*
*/
void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16)
      Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i > 0)
      Serial.print(":");
  }
  Serial.println();
}

/**
*   Makes AP and, when client connected, serves the 
*   web page with entry fields. The fields are 
*   returned in the parameters.
*   
*   @param ssid, a String to place the ssid
*   @param passcode, a String to place the
*   @param gsid, a String to place the gsid
*/
void AP_getInfo(String &ssid, String &passcode, String &gsid) {
  WiFiServer server(80);
  WiFiClient client;
  Serial.println(F("Access Point Web Server"));

  status = WiFi.status();
  if (status == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    while (true)
      ;
  }

  // make AP with string+MAC address
  makeMACssidAP("csl");

  // wait for connection:
  delay(1000);
  printWiFiStatus();
  
  // Start the server - THIS IS IMPORTANT
  server.begin();
  Serial.println("Server started");

  while (true) {  // loop to poll connections etc.
    // compare the previous status to the current status
    if (status != WiFi.status()) {  // someone joined AP or left
      // it has changed update the variable
      status = WiFi.status();
      if (status == WL_AP_CONNECTED) {
        byte remoteMac[6];
        Serial.print(F("Device connected to AP, MAC address: "));
        WiFi.APClientMacAddress(remoteMac);
        printMacAddress(remoteMac);

        // print where to go in a browser:
        IPAddress ip = WiFi.localIP();
        Serial.print(F("To provide provisioning info, open a browser at http://"));
        Serial.println(ip);

        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Provision access");
        display.print("at http://");
        display.println(ip);
        display.display();
      } else {
        // a device has disconnected from the AP, and we are back in listening mode
        Serial.println(F("Device disconnected from AP"));
        display.println("Device disconnected");
        display.display();
      }
    }

    client = server.available();  // listen for incoming clients

    if (client) {                    // if you get a client,
      Serial.println(F("new client"));  // print a message out the serial port
      String currentLine = "";       // make a String to hold incoming data from the client
      bool currentLineIsBlank = true;
      unsigned long connectionStart = millis();
      
      while (client.connected() && millis() - connectionStart < 5000) {   // loop while the client's connected with timeout
        if (client.available()) {    // if there's bytes to read from the client,
          char c = client.read();    // read a byte, then
          Serial.write(c);           // print it out the serial monitor
          
          if (c == '\n' && currentLineIsBlank) {
            // This is where we send the HTTP response
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println(webpage_html);
            break;
          }
          
          if (c == '\n') {
            // If we get here, we reached the end of a line
            currentLineIsBlank = true;
            currentLine = "";
            
            // Check if line was a GET request with form data
            if (currentLine.startsWith("GET /get?")) {
              int ssidIndx = currentLine.indexOf("SSID=");
              int passcodeIndx = currentLine.indexOf("passcode=");
              int gsidIndx = currentLine.indexOf("GSID=");
              int httpIndx = currentLine.indexOf(" HTTP");
              
              if (ssidIndx > 0 && passcodeIndx > 0 && gsidIndx > 0 && httpIndx > 0) {
                ssid = urlDecode(currentLine.substring(ssidIndx + 5, passcodeIndx - 1));
                passcode = urlDecode(currentLine.substring(passcodeIndx + 9, gsidIndx - 1));
                gsid = urlDecode(currentLine.substring(gsidIndx + 5, httpIndx));
                
                // Respond with success page
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/html");
                client.println("Connection: close");
                client.println();
                client.println("<html><body><h1>Configuration Saved</h1><p>Your settings have been saved.</p></body></html>");
                
                // close the connection
                client.stop();
                Serial.println("Form data received and connection closed");
                WiFi.end();
                delay(1000);
                storeinfo(ssid, passcode, gsid);
                return;
              }
            }
          } else if (c != '\r') {
            // If we get any other character, add it to currentLine
            currentLineIsBlank = false;
            currentLine += c;
          }
        }
      }
      
      // Close the connection
      client.stop();
      Serial.println(F("Client disconnected"));
    }
  }
}

/**
*   Create an AP with a unique ssid formed with a string
*   and the last 2 hex digits of the board MAC address.
*   By default the local IP address of will be 192.168.1.1
*   you can override it with the following:
*   WiFi.config(IPAddress(10, 0, 0, 1));
*
*   @param startString a string to preface the ssid
*/
void makeMACssidAP(String startString) {

  byte localMac[6];

  Serial.print(F("Device MAC address: "));
  WiFi.macAddress(localMac);
  printMacAddress(localMac);

  char myHexString[3];
  sprintf(myHexString, "%02X%02X", localMac[1], localMac[0]);
  String ssid = startString + String((char *)myHexString);

  Serial.print(F("Creating access point: "));
  Serial.println(ssid);

  status = WiFi.beginAP(ssid.c_str());

  if (status != WL_AP_LISTENING) {
    Serial.println(F("Creating access point failed"));
    while (true)
      ;
  }
}