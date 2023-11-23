#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

#include "DHT.h"

const char* ssid = "XXX";
const char* password = "XXX";

IPAddress staticIP(192, 168, 1, 150); //<<<<<<<<<<<<<<
IPAddress gateway(192, 168, 1, 1);    // Gateway 
IPAddress subnet(255, 255, 255, 0);   // Subnet mask


#define LED 2
#define DHTPIN 16       
#define DHTTYPE DHT11  
#define RELAY_1 17    
#define RELAY_2 18    

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

bool relay1Status = false; // Track status of Relay 1
bool relay2Status = false; // Track status of Relay 2

bool autoMode = true; // Variable to store the current mode
float autoTempThreshold = 8.0; // Temperature threshold for automatic control

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  //digitalWrite(LED, HIGH);
  pinMode(RELAY_1, OUTPUT);
  digitalWrite(RELAY_1, HIGH);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_2, HIGH);
  dht.begin();

  WiFi.begin(ssid, password);
  // Set static IP
  WiFi.config(staticIP, gateway, subnet);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggleLamp", handleToggleLamp);
  server.on("/toggleFan", handleToggleFan);
  server.on("/setMode", handleSetMode);
  server.on("/setThreshold", handleSetThreshold);
  server.on("/data", sendSensorData);
  
  server.begin();
}

void loop() {
  server.handleClient();
  if(autoMode) {
    float t = dht.readTemperature();
    controlRelays(t);
    
  }
  sendDataToServer();
  checkForAlerts();
  
}

void controlRelays(float t) {
  if(autoMode) {
    relay1Status = t < autoTempThreshold;
    relay2Status = t < autoTempThreshold;

    digitalWrite(RELAY_1, relay1Status ? LOW : HIGH);
    digitalWrite(RELAY_2, relay2Status ? LOW : HIGH);
  }
}


void controlRelays1(float t) {
  if (t < autoTempThreshold) {
    digitalWrite(RELAY_1, LOW); // Turn on IR lamp
    digitalWrite(RELAY_2, LOW); // Turn on Fan
  } else {
    digitalWrite(RELAY_1, HIGH); // Turn off IR lamp
    digitalWrite(RELAY_2, HIGH); // Turn off Fan
  }
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Greenhouse Controller</h1>";
  html += "<p><a href=\"/toggleLamp\">Toggle IR Lamp</a></p>";
  html += "<p><a href=\"/toggleFan\">Toggle Fan</a></p>";
  html += "<form action=\"/setMode\" method=\"get\">";
  html += "<input type=\"submit\" name=\"mode\" value=\"auto\" />";
  html += "<input type=\"submit\" name=\"mode\" value=\"manual\" />";
  html += "</form>";
  html += "<form action=\"/setThreshold\" method=\"get\">";
  html += "<input type=\"number\" name=\"threshold\" step=\"0.1\" />";
  html += "<input type=\"submit\" value=\"Set Threshold\" />";
  html += "</form>";
  html += "<div id='sensorData'><p>Loading sensor data...</p></div>";
  html += "<script>setInterval(function() {fetch('/data').then(response => response.text()).then(data => document.getElementById('sensorData').innerHTML = data);}, 2000);</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleToggleLamp() {
  if (!autoMode) {
    relay1Status = !relay1Status;
    digitalWrite(RELAY_1, relay1Status ? LOW : HIGH);
  }
  server.sendHeader("Location", "/"); // Redirect to root to update the page immediately
  server.send(303); // HTTP response code for See Other, causing a redirect
}

void handleToggleFan() {
  if (!autoMode) {
    relay2Status = !relay2Status;
    digitalWrite(RELAY_2, relay2Status ? LOW : HIGH);
  }
  server.sendHeader("Location", "/"); // Redirect to root to update the page immediately
  server.send(303); // HTTP response code for See Other, causing a redirect
}

void handleSetMode() {
  if (server.hasArg("mode")) {
    autoMode = (server.arg("mode") == "auto");
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetThreshold() {
  if (server.hasArg("threshold")) {
    autoTempThreshold = server.arg("threshold").toFloat();
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void sendDataToServer() {
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastTime > 5*60000) { // X minutes interval
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    String serverPath = "http://192.168.1.88/Monitoring/GREEN/data_logger.php?temperature=" + String(t) + "&humidity=" + String(h);

    WiFiClient client;
    HTTPClient http;

    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();

    lastTime = currentTime;
  }
}
void sendSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  String data = "<p>Humidity: " + String(h) + "%</p>";
  data += "<p>Temperature: " + String(t) + "°C</p>";
  data += "<p>IR Lamp: " + String(relay1Status ? "ON" : "OFF") + "</p>";
  data += "<p>Fan: " + String(relay2Status ? "ON" : "OFF") + "</p>";
  
  data += "<p>Mode: " + String(autoMode ? "Automatic" : "Manual") + "</p>";
  data += "<p>Threshold: " + String(autoTempThreshold) + "°C</p>";
  server.send(200, "text/plain", data);
}

void checkForAlerts() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    blinkLED(5); // Blink 5 times if sensor error
  }
  else if (t > 26 || t < 8) {
    blinkLED(3); // Blink 3 times for temperature alert
  }
}

void blinkLED(int blinks) {
  for (int i = 0; i < blinks; i++) {
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
  }
}
