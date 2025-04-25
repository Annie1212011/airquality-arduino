/*
  Write to Google Sheets through a Wifi  HTTP1.1 request to a Google script.
*/
void payloadUpload(String payload) {
  for (int i = 1; i < 4; i++) { // allways try to connect to wifi
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
      // Make a HTTP request:
      client.println(String("POST /airquality/?gsid=") + String(gsidg) + String(" HTTP/1.1"));
      client.println("Host: https://annie1212011.github.io");
      client.println("Content-Type: application/x-www-form-urlencoded");
      //client.println("Connection: close");
      client.print("Content-Length: ");
      client.println(payload.length());
      client.println();
      client.print(payload);
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
  if (client.connectSSL(server, 443)) {      // if you get a connection, report back via serial:
    Serial.print("Connected to "); Serial.println(server);
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
    client.print("GET /your-repo-name/?gsid=");
    client.print(gsidg);
    client.println(" HTTP/1.1");
    
    client.print("Host: ");
    client.println(server);
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println(); // Empty line to end headers
  }
  else {
    Serial.print("Not connected to ");
    Serial.println(server);
  }
  Serial.println("end intializeClient");
}
