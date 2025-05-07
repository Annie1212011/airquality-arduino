/*
  Send data to ThingSpeak, which can be integrated with Google Sheets
*/
void payloadUpload(String payload) {
  Serial.println("Attempting to connect to WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssidg);
  
  // Initialize WiFi radio
  WiFi.end();
  delay(1000);
  
  for (int i = 1; i < 4; i++) {
    Serial.print("Connection attempt #");
    Serial.println(i);
    
    status = WiFi.begin(ssidg, passcodeg);
    
    Serial.print("WiFi begin status: ");
    Serial.println(status);
    
    // Wait for connection
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 6000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Successfully connected to WiFi!");
      IPAddress ip = WiFi.localIP();
      Serial.print("IP Address: ");
      Serial.print(ip[0]); Serial.print(".");
      Serial.print(ip[1]); Serial.print(".");
      Serial.print(ip[2]); Serial.print(".");
      Serial.println(ip[3]);
      
      // Define ThingSpeak server
      char server[] = "api.thingspeak.com";
      
      if (client.connectSSL(server, 443)) {
        Serial.print("Connected to ");
        Serial.println(server);
        
        // Parse your payload string using the proper index positions
        // Your payload format from the header: 
        // "DateTime, CO2, Tco2, RHco2, Tbme, Pbme, RHbme, vbat(mV), status, mP1.0, mP2.5, mP4.0, mP10, ..."
        
        // Extract the values at the correct positions
        String sensorDateTime = extractValue(payload, 0);  // DateTime
        String co2Value = extractValue(payload, 1);        // CO2 ppm
        String tempValue = extractValue(payload, 2);       // Tco2 (temp from CO2 sensor)
        String humidityValue = extractValue(payload, 3);   // RHco2 (humidity from CO2 sensor)
        
        // PM2.5 is at the 10th position (index 10) after the header
        String pm25Value = extractValue(payload, 10);      // mP2.5
        
        // Using global variables for VOC and NOX that were updated in readSen5x()
        String vocValue = String(Voc);
        String noxValue = String(Nox);
        
        // Debug printing
        Serial.println("Sending to ThingSpeak:");
        Serial.println("DateTime: " + sensorDateTime);
        Serial.println("CO2: " + co2Value);
        Serial.println("Temp: " + tempValue);
        Serial.println("Humidity: " + humidityValue);
        Serial.println("PM2.5: " + pm25Value);
        Serial.println("VOC: " + vocValue);
        Serial.println("NOx: " + noxValue);
        
        // Create HTTP POST request for ThingSpeak
        String postData = "api_key=2WAW5DVDUKLSBJ9Z";  // Replace with your actual API key
        postData += "&field1=" + co2Value;
        postData += "&field2=" + tempValue;
        postData += "&field3=" + humidityValue;
        postData += "&field4=" + pm25Value;
        postData += "&field5=" + vocValue;
        postData += "&field6=" + noxValue;
        
        client.println("POST /update HTTP/1.1");
        client.print("Host: ");
        client.println(server);
        client.println("User-Agent: Arduino/1.0");
        client.println("Connection: close");
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-Length: ");
        client.println(postData.length());
        client.println();
        client.println(postData); // Send the data
        
        // Wait for response with timeout
        unsigned long responseTimeout = millis() + 10000;
        while (!client.available() && millis() < responseTimeout) {
          delay(100);
        }

        if (client.available()) {
          Serial.println("Response received:");
          
          // Read all headers first
          String responseHeaders = "";
          while (client.available()) {
            String line = client.readStringUntil('\n');
            responseHeaders += line + "\n";
            if (line.length() <= 2) { // Empty line signals end of headers
              break;
            }
          }
          Serial.println(responseHeaders);
          
          // Read response body
          String responseBody = "";
          while (client.available()) {
            char c = client.read();
            responseBody += c;
          }
          
          // Check for successful response (should contain a "1" for success)
          if (responseBody.indexOf("1") >= 0) {
            Serial.println("Data successfully uploaded to ThingSpeak!");
          } else {
            Serial.println("ThingSpeak upload may have failed. Response:");
            Serial.println(responseBody);
          }
        } else {
          Serial.println("No response within timeout");
        }

        client.stop();
        Serial.println("Connection closed");
        WiFi.end();
        return;
      } else {
        Serial.print("Failed to connect to ");
        Serial.println(server);
      }
    } else {
      Serial.print("Failed to connect to WiFi. Status: ");
      Serial.println(WiFi.status());
    }
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Continuing without WiFi");
  }
}

// Improved helper function to extract values from the payload string
String extractValue(String payload, int fieldNumber) {
  // Count commas to find the field position
  int commaCount = 0;
  int startPos = 0;
  
  // Handle the case of the first field (before first comma)
  if (fieldNumber == 0) {
    int firstComma = payload.indexOf(',');
    if (firstComma >= 0) {
      return payload.substring(0, firstComma).trim();
    } else {
      return payload.trim(); // No commas found, return entire string
    }
  }
  
  // For other fields, find the position between commas
  for (int i = 0; i < payload.length(); i++) {
    if (payload.charAt(i) == ',') {
      commaCount++;
      
      if (commaCount == fieldNumber) {
        startPos = i + 1; // Start after this comma
      }
      else if (commaCount == fieldNumber + 1) {
        return payload.substring(startPos, i).trim(); // Return between commas
      }
    }
  }
  
  // If we reached the end of the string and found the starting position
  // but not the ending comma, this is the last field
  if (commaCount >= fieldNumber) {
    return payload.substring(startPos).trim();
  }
  
  // Field not found
  return "";
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

// URL encoding function (kept for backward compatibility)
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