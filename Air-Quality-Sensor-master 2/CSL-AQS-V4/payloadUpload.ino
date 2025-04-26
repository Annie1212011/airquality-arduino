void payloadUpload(String payload) {
  Serial.println("==== STARTING PAYLOAD UPLOAD ====");
  Serial.println("Attempting to connect to WiFi...");
  
  // Initialize WiFi
  WiFi.end();
  delay(1000);
  
  // Generate the payload
  String jsonPayload = payload_base + String("\"") + payload + String("\"}");
  Serial.println("Base payload: " + jsonPayload);
  
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
      
      // Connect to script.googleusercontent.com which allows HTTP
      WiFiClient client;
      char scriptServer[] = "script.googleusercontent.com";
      
      Serial.print("Connecting to: ");
      Serial.println(scriptServer);
      
      if (client.connect(scriptServer, 80)) {
        Serial.println("Connected successfully");
        
        // Format as URL-encoded form data
        String postData = "payload=" + urlEncode(jsonPayload);
        
        // Build the request path - this is the key part that makes it work
        String path = "/macros/echo?user_content_key=YOUR_USER_CONTENT_KEY"
                      "&devmode=true&urlfetchtest=true"
                      "&gsid=" + String(gsidg);
        
        // Send the HTTP POST request
        client.println("POST " + path + " HTTP/1.1");
        client.println("Host: " + String(scriptServer));
        client.println("User-Agent: Arduino/1.0");
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-Length: ");
        client.println(postData.length());
        client.println("Connection: close");
        client.println();
        client.println(postData);
        
        // Wait for response
        unsigned long responseTimeout = millis() + 10000;
        while (!client.available() && millis() < responseTimeout) {
          delay(100);
        }

        if (client.available()) {
          Serial.println("Response received:");
          
          // Read just the first part of the response for debugging
          for (int i = 0; i < 30 && client.available(); i++) {
            char c = client.read();
            Serial.write(c);
          }
          
          // Just note if there's more
          if (client.available()) {
            Serial.println("... (response truncated)");
          }
        } else {
          Serial.println("No response within timeout");
        }

        client.stop();
        Serial.println("Connection closed");
        WiFi.end();
        return;
      } else {
        Serial.println("Failed to connect to server");
        
        // Alternative: Try using Webhook.site as a logging service
        char webhookSite[] = "webhook.site";
        String webhookID = "YOUR_WEBHOOK_ID"; // Get this from webhook.site
        
        if (client.connect(webhookSite, 80)) {
          Serial.println("Connected to webhook.site for logging");
          
          // URL encode the data in the path
          String requestPath = "/YOUR_WEBHOOK_ID?data=" + urlEncode(jsonPayload);
          
          client.println("GET " + requestPath + " HTTP/1.1");
          client.println("Host: " + String(webhookSite));
          client.println("Connection: close");
          client.println();
          
          // Wait briefly for response but don't care about content
          delay(1000);
          client.stop();
          Serial.println("Logged data to webhook.site");
          WiFi.end();
          return;
        }
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