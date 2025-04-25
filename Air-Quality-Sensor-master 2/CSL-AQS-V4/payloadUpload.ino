/*
  Write to Google Sheets through a Wifi HTTP request to GitHub Pages proxy.
*/
void payloadUpload(String payload) {
  for (int i = 1; i < 4; i++) { // always try to connect to wifi
    if (passcodeg != "") // if password is not empty
      status = WiFi.begin(ssidg, passcodeg);
    else
      status = WiFi.begin(ssidg);
    delay(500);

    if (WiFi.status() == WL_CONNECTED) {
      if (!client.connected()) {
        initializeClient();
      }
      Serial.print("payload: ");
      payload = payload_base + String("\"") + payload + String("\"}");
      Serial.println(payload);
      
      // URL encode the payload
      String encodedPayload = urlEncode(payload);
      
      // Make a HTTP request:
      client.println("GET /airquality/?gsid=" + String(gsidg) + "&payload=" + encodedPayload + " HTTP/1.1");
      client.println("Host: annie1212011.github.io");
      client.println("User-Agent: Arduino/1.0");
      client.println("Connection: close");
      client.println();
      
      delay(200);

      if (client.available())
        Serial.println("Response: ");
      while (client.available()) {
        char c = client.read();
        Serial.write(c);
      }

      client.stop();
      if (!client.connected()) {
        Serial.println("disconnected from server");
      };
      WiFi.end();
      break;
    }
    else {
      Serial.print("Trying to connect to Wifi : "); Serial.println(i);
    }
  }
  if (status != WL_CONNECTED)
    Serial.println("Continuing without WiFi");
}

void initializeClient() {
  Serial.print("\nStarting connection to server... ");
  if (client.connectSSL(server, 443)) {
    Serial.print("Connected to "); Serial.println(server);
    // Initial connection is successful, don't need to do anything else here
  }
  else {
    Serial.print("Not connected to ");
    Serial.println(server);
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
    } else if (isAlphaNumeric(c)) {
      encodedString += c;
    } else {
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