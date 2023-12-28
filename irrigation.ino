#include <SPI.h>
#include <WiFiNINA.h>

// Setup network credentials
char ssid[] = "your_network_name";
char pass[] = "your_password";

int status = WL_IDLE_STATUS;
WiFiServer server(80);

const int relayPin = 7;
unsigned long interval = 86400000; // 24 hours in milliseconds
unsigned long previousMillis = 0;
unsigned long lastWateringTime = 0;
bool skipNext = false;

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("/info") != -1) {
      sendInfo(client);
    } else if (request.indexOf("/start") != -1) {
      handleStart(client);
    } else if (request.indexOf("/skip") != -1) {
      handleSkip(client);
    }

    client.stop();
  }

  unsigned long currentMillis = millis();
  if (!skipNext && currentMillis - previousMillis >= interval) {
    startWatering();
    previousMillis = currentMillis;
  } else if (skipNext && currentMillis - previousMillis >= interval) {
    skipNext = false;
    previousMillis = currentMillis;
  }
}

void startWatering() {
  digitalWrite(relayPin, HIGH);
  delay(5000); // Water for 5 seconds
  digitalWrite(relayPin, LOW);
  lastWateringTime = millis();
}

void sendInfo(WiFiClient client) {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastWateringTime;

  int hours = int(elapsedTime / 3600000);
  int mins = int((elapsedTime % 3600000) / 60000);
  int secs = int((elapsedTime % 60000) / 1000);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.print("{\"lastWateringTime\":\"");
  client.print(hours);
  client.print(":");
  if (mins < 10) client.print("0");
  client.print(mins);
  client.print(":");
  if (secs < 10) client.print("0");
  client.print(secs);
  client.println("\"}");
}

void handleStart(WiFiClient client) {
  startWatering();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{\"message\":\"Watering started.\"}");
}

void handleSkip(WiFiClient client) {
  skipNext = true;
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{\"message\":\"Next watering skipped.\"}");
}
