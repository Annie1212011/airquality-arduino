void payloadUpload(String sensorData) {
  Serial.println("==== STARTING PAYLOAD UPLOAD ====");
  
  // Initialize WiFi
  WiFi.end();
  delay(1000);
  
  for (int i = 1; i < 4; i++) {
    Serial.println("Connection attempt #" + String(i));
    
    status = WiFi.begin(ssidg, passcodeg);
    
    // Wait for connection
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 6000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Successfully connected to WiFi!");
      
      // Parse your sensor data
      // Format from your existing code: Date/Time, CO2, Tco2, RHco2, Tbme, Pbme, RHbme, vbat, status, ...
      String data = sensorData;
      
      // Skip the date/time value (first value)
      int firstComma = data.indexOf(',');
      data = data.substring(firstComma + 1);
      
      // Parse out individual values
      String co2Value = getValue(data, ',', 0);
      String tempValue = getValue(data, ',', 1);
      String humidityValue = getValue(data, ',', 2);
      
      // Skip next 3 values (Tbme, Pbme, RHbme)
      String voltageValue = getValue(data, ',', 6);
      String statusValue = getValue(data, ',', 7);
      
      // Connect to ThingSpeak
      WiFiClient client;
      const char* thingSpeakServer = "api.thingspeak.com";
      
      Serial.print("Connecting to ThingSpeak... ");
      
      if (client.connect(thingSpeakServer, 80)) {
        Serial.println("Connected!");
        
        // Your ThingSpeak Write API Key
        String apiKey = "YOUR_API_KEY_HERE";
        
        // Build the request URL
        String url = "/update?api_key=" + apiKey;
        url += "&field1=" + co2Value;
        url += "&field2=" + tempValue;
        url += "&field3=" + humidityValue;
        url += "&field4=" + voltageValue;
        url += "&field5=" + statusValue;
        
        // Add more fields as needed for your additional sensor data
        
        Serial.println("Sending data to ThingSpeak: " + url);
        
        // Send the HTTP request
        client.print("GET " + url + " HTTP/1.1\r\n");
        client.print("Host: api.thingspeak.com\r\n");
        client.print("Connection: close\r\n\r\n");
        
        // Wait for response
        unsigned long responseTimeout = millis() + 5000;
        while (!client.available() && millis() < responseTimeout) {
          delay(100);
        }

        if (client.available()) {
          Serial.println("Response received:");
          
          // Read headers
          String line = "";
          while (client.available()) {
            char c = client.read();
            if (c == '\n') {
              Serial.println(line);
              line = "";
            } else if (c != '\r') {
              line += c;
            }
          }
        } else {
          Serial.println("No response within timeout");
        }

        client.stop();
        Serial.println("Connection closed");
        WiFi.end();
        return;
      } else {
        Serial.println("Failed to connect to ThingSpeak");
      }
    } else {
      Serial.print("Failed to connect to WiFi. Status: ");
      Serial.println(WiFi.status());
    }
  }
  
  Serial.println("==== PAYLOAD UPLOAD FAILED ====");
  Serial.println("Continuing without WiFi");
}

// Helper function to parse CSV values
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
void initializeClient() {
  char netlifyServer[] = "harbor-airquality.netlify.app";
  
  Serial.print("\nStarting connection to server... ");
  if (client.connectSSL(netlifyServer, 443)) {
    Serial.print("Connected to ");
    Serial.println(netlifyServer);
  }
  else {
    Serial.print("Not connected to ");
    Serial.println(netlifyServer);
  }
  Serial.println("end intializeClient");
}
// URL encoding function
String urlEncode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (c == '\"') {
      // Properly encode double quotes
      encodedString += "%22";
    } else if (c == '{' || c == '}' || c == ',' || c == ':') {
      // Properly encode JSON structural characters
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    } else if (isAlphaNumeric(c) || c == '.' || c == '-' || c == '_') {
      // Safe characters that don't need encoding
      encodedString += c;
    } else {
      // All other characters
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}