/*
  Send data to Google Sheets through Pipedream
*/
void payloadUpload(String payload) {
  Serial.println("Attempting to connect to WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssidg);
  
  // Initialize WiFi radio
  WiFi.end();
  delay(1000);
  
  // Format the data as JSON
  String jsonPayload = "{\"data\":[" + payload + "]}";
  Serial.println("Payload: " + jsonPayload);
  
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
      
      // Define Pipedream endpoint
      // Replace with your Pipedream webhook URL
      char server[] = "endpoint.m.pipedream.net"; 
      
      if (client.connectSSL(server, 443)) {
        Serial.print("Connected to ");
        Serial.println(server);
        
        // Create HTTP POST request
        client.println("POST /your-pipedream-id HTTP/1.1"); // Replace your-pipedream-id with your actual endpoint ID
        client.print("Host: ");
        client.println(server);
        client.println("User-Agent: Arduino/1.0");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(jsonPayload.length());
        client.println("Connection: close");
        client.println();
        client.println(jsonPayload); // Send the JSON payload
        
        // Wait for response
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
          
          // Print response body (truncated if too long)
          if (responseBody.length() > 200) {
            Serial.println(responseBody.substring(0, 200) + "...");
          } else {
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