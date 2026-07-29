#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

void handleRoot() {
  server.send(200, "text/html",
    "<form action='/save'>"
    "SSID:<br><input name='ssid'><br>"
    "Haslo:<br><input name='pass' type='password'><br><br>"
    "<input type='submit' value='Zapisz'>"
    "</form>");
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + pass);

  // tutaj można zapisać do Preferences

  server.send(200, "text/html", "Zapisano. Restart...");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Konfiguracja", "12345678");

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();

  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}
