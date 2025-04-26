/*
  Write to Google Sheets through a Netlify Function proxy.
*/
void payloadUpload(String payload) {
  Serial.println("Attempting to connect to WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssidg);
  Serial.print("Password length: ");
  Serial.println(passcodeg.length());
  
  // Initialize WiFi radio
  WiFi.end();
  delay(1000);
  
  // Generate the payload only once before connection attempts
  String jsonPayload = payload_base + String("\"") + payload + String("\"}");
  Serial.println("Base payload: " + jsonPayload);
  String encodedPayload = urlEncode(jsonPayload);
  
  for (int i = 1; i < 4; i++) { // always try to connect to wifi
    Serial.print("Connection attempt #");
    Serial.println(i);
    
    if (passcodeg != "") // if password is not empty
      status = WiFi.begin(ssidg, passcodeg);
    else
      status = WiFi.begin(ssidg);
    
    // Print the WiFi status code
    Serial.print("WiFi begin status: ");
    Serial.println(status);
    
    // Wait longer for connection
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
      Serial.print(ip[0]);
      Serial.print(".");
      Serial.print(ip[1]);
      Serial.print(".");
      Serial.print(ip[2]);
      Serial.print(".");
      Serial.println(ip[3]);
      
      // Try connecting to GitHub Pages instead for testing
      char server[] = "annie1212011.github.io";
      
      Serial.print("Attempting to connect to ");
      Serial.println(server);
      
      if (client.connectSSL(server, 443)) {
        Serial.print("Connected successfully to ");
        Serial.println(server);
        
        // Make HTTP request to GitHub Pages as a test
        client.print("GET /airquality/?gsid=");
        client.print(gsidg);
        client.print("&payload=");
        client.print(encodedPayload);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(server);
        client.println("User-Agent: Arduino/1.0");
        client.println("Connection: close");
        client.println();
        
        // Wait for response with timeout
        unsigned long responseTimeout = millis() + 10000; // 10 second timeout
        while (!client.available() && millis() < responseTimeout) {
          delay(100);
        }

        if (client.available()) {
          Serial.println("Response received: ");
          // Read the entire response
          while (client.available()) {
            char c = client.read();
            Serial.write(c);
          }
        } else {
          Serial.println("No response received within timeout");
        }

        client.stop();
        Serial.println("Connection closed");
        WiFi.end();
        return; // Exit after successful attempt
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