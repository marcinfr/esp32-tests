#include <WiFi.h>

const char* ssid = "Nie mam internetu !";
const char* password = "lubiepiwo";

WiFiServer server(80);

const int LED_WHITE = 22;
const int LED_RED = 23;

int LED_RED_ENABLED = -1;
int LED_WHITE_ENABLED = -1;

void setup() {
  pinMode(LED_WHITE, OUTPUT);
  digitalWrite(LED_WHITE, LOW);

  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, LOW);

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Laczenie");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Polaczono!");
  Serial.print("Adres IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (!client)
    return;

  while (!client.available())
    delay(1);

  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("/ON_WHITE") != -1) {
    if (LED_WHITE_ENABLED < 0) {
      digitalWrite(LED_WHITE, HIGH);
    } else {
      digitalWrite(LED_WHITE, LOW);
    }
    LED_WHITE_ENABLED = (-1) * LED_WHITE_ENABLED;
  }

  if (request.indexOf("/ON_RED") != -1) {
     if (LED_RED_ENABLED < 0) {
      digitalWrite(LED_RED, HIGH);
    } else {
      digitalWrite(LED_RED, LOW);
    }
    LED_RED_ENABLED = (-1) * LED_RED_ENABLED;
  }

  if (request.indexOf("/OFF") != -1) {
    digitalWrite(LED_WHITE, LOW);
    digitalWrite(LED_RED, LOW);
    LED_RED_ENABLED = -1;
    LED_WHITE_ENABLED = -1;
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();

  client.println("<!DOCTYPE html>");
  client.println("<html>");
  client.println("<head>");
  client.println("<meta charset='UTF-8'>");
  client.println("<title>ESP32</title>");
  client.println("</head>");
  client.println("<body style='text-align:center;font-family:Arial;'>");

  client.println("<h1>Sterowanie ESP32</h1>");

  client.println("<p><a href='/ON_RED'><button style='width:200px;height:60px;font-size:24px;'>LED czerwony</button></a></p>");
  client.println("<p><a href='/ON_WHITE'><button style='width:200px;height:60px;font-size:24px;'>LED biały</button></a></p>");

  client.println("<p><a href='/OFF'><button style='width:200px;height:60px;font-size:24px;'>Wyłącz LEDy</button></a></p>");

  client.println("</body>");
  client.println("</html>");

  delay(1);
  client.stop();
}
