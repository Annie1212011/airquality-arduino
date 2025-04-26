/*
  Write to Google Sheets through a Netlify Function proxy.
*/
void payloadUpload(String payload) {
  Serial.println("Attempting to connect to WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssidg);
  
  // Initialize WiFi radio
  WiFi.end();
  delay(1000);
  
  // Generate the payload only once before connection attempts
  String jsonPayload = payload_base + String("\"") + payload + String("\"}");
  Serial.println("Base payload: " + jsonPayload);
  
  // URL encode the payload for transmission
  String encodedPayload = urlEncode(jsonPayload);
  
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
      
      // Define server
      char server[] = "harbor-airquality.netlify.app"; // Remove trailing slash
      
      if (client.connectSSL(server, 443)) {
        Serial.print("Connected to ");
        Serial.println(server);
        
        // Debug GSID
        Serial.print("GSID: ");
        Serial.println(gsidg);
        
        // Format the HTTP request - note the spaces in the URL between parameters
        String requestUrl = "/api/proxy?gsid=" + String(gsidg) + "&payload=" + encodedPayload;
        Serial.print("Request URL length: ");
        Serial.println(requestUrl.length());
        
        // If URL is very long, print just the beginning
        if (requestUrl.length() > 100) {
          Serial.print("Request URL (truncated): ");
          Serial.println(requestUrl.substring(0, 100) + "...");
        } else {
          Serial.print("Request URL: ");
          Serial.println(requestUrl);
        }
        
        // Send the request with proper headers
        client.print("GET ");
        client.print(requestUrl);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(server);
        client.println("User-Agent: Arduino/1.0");
        client.println("Connection: close");
        client.println();
        
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