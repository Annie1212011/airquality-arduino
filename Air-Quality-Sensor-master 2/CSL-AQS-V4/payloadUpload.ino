void payloadUpload(String payload) {
  Serial.println("==== STARTING PAYLOAD UPLOAD ====");
  Serial.println("Attempting to connect to WiFi...");
  
  // Initialize WiFi
  WiFi.end();
  delay(1000);
  
  // Generate the payload
  String jsonPayload = payload_base + String("\"") + payload + String("\"}");
  Serial.println("Base payload: " + jsonPayload);
  String encodedPayload = urlEncode(jsonPayload);
  
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
      IPAddress ip = WiFi.localIP();
      Serial.print("IP Address: ");
      Serial.print(ip[0]); Serial.print(".");
      Serial.print(ip[1]); Serial.print(".");
      Serial.print(ip[2]); Serial.print(".");
      Serial.println(ip[3]);
      
      // Use non-SSL connection to GitHub Pages
      WiFiClient client;
      char githubServer[] = "annie1212011.github.io";
      
      Serial.print("Connecting to GitHub Pages: ");
      Serial.println(githubServer);
      
      if (client.connect(githubServer, 80)) {
        Serial.println("Connected to GitHub Pages");
        
        // Format the request URL - simple GET request with parameters
        String requestUrl = "/airquality/?gsid=" + String(gsidg) + "&payload=" + encodedPayload;
        
        Serial.println("Sending request: " + requestUrl);
        
        // Send the HTTP request
        client.print("GET ");
        client.print(requestUrl);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(githubServer);
        client.println("User-Agent: Arduino/1.0");
        client.println("Connection: close");
        client.println();
        
        // Wait for response
        unsigned long responseTimeout = millis() + 10000;
        while (!client.available() && millis() < responseTimeout) {
          delay(100);
        }

        // Read the response (just for logging)
        if (client.available()) {
          Serial.println("Response received:");
          
          // Read all headers first
          String responseHeaders = "";
          bool headerEnded = false;
          while (client.available() && !headerEnded) {
            String line = client.readStringUntil('\n');
            if (line.length() <= 2) {
              headerEnded = true;
            } else {
              responseHeaders += line + "\n";
            }
          }
          Serial.println(responseHeaders);
          
          // Read response body (limited)
          int bodyLength = 0;
          while (client.available() && bodyLength < 200) {
            char c = client.read();
            Serial.write(c);
            bodyLength++;
          }
          
          // If there's more data, just note it but don't print
          if (client.available()) {
            Serial.println("(response truncated)");
            while (client.available()) {
              client.read(); // Clear the buffer
            }
          }
        } else {
          Serial.println("No response received within timeout");
        }

        client.stop();
        Serial.println("Connection closed");
        WiFi.end();
        return;
      } else {
        Serial.println("Failed to connect to GitHub Pages");
      }
    } else {
      Serial.print("Failed to connect to WiFi. Status: ");
      Serial.println(WiFi.status());
    }
  }
  
  Serial.println("==== PAYLOAD UPLOAD FAILED ====");
  Serial.println("Continuing without WiFi");
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