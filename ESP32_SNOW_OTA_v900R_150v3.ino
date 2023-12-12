#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include "DHT.h"
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <Update.h>

#include <map>
std::map<String, float> peerTemperatures; // Map to store temperatures from peers

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
const char* ssid = "XXX";
const char* password = "XXX";

IPAddress staticIP(192, 168, 1, 150); //<<<<<<<<<<<<<<
IPAddress gateway(192, 168, 1, 1);    // Gateway 
IPAddress subnet(255, 255, 255, 0);   // Subnet mask

float autoTempThreshold = 8.0;
float maxTemp = 25.0;
float lowTemp = 12.0;

// Restart interval: 1 hour (3600000 milliseconds)
const unsigned long restartInterval = 15 * 60000; //<<<<<<<<<<<<<<
// Seding Data
const unsigned long sendData2peers = 3 * 60000; //<<<<<<<<<<<<<<
const unsigned long sendData2Servers = 6 * 60000; //<<<<<<<<<<<<<<
// ON or OFF time
const unsigned long lampDuration = 0 * 60000; //<<<<<<<<<<<<<<
const unsigned long fanDuration = 0 * 60000; //<<<<<<<<<<<<<<


// Unique name for each device
String deviceName = "Greenhouse150"; //<<<<<<<<<<<<<<
const char* host = "Greenhouse150";//<<<<<<<<<<<<<<
String folder = "0";//<<<<<<<<<<<<<<

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// GPIO pin configuration
#define LED 2
#define DHTPIN 16
#define DHTTYPE DHT11
#define RELAY_1 17
#define RELAY_2 18


unsigned long lampStartTime = 0;
unsigned long fanStartTime = 0;
bool lampOn = false;
bool fanOn = false;

unsigned long lastRestartTime = 0;

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);



// Global variables
bool relay1Status = false;
bool relay2Status = false;
bool autoMode = true;

/*
 * Login page
 */

const char* loginIndex =
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
             "<td>Username:</td>"
             "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";

/*
 * Server Index Page
 */

const char* serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";
 

void setup() {
  Serial.begin(115200);
  setupGPIO();
  setupDHT();
  setupWiFi();
  setupWebServer();
  
  // Initialize the last restart time
  lastRestartTime = millis(); 

}

void loop() {
  server.handleClient();

  static unsigned long lastSendTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastSendTime > sendData2peers ) { //<<<<<<<<<<<<<<<<< Send data every X minutes
        float localTemp = dht.readTemperature();
        if (localTemp != -1) { // Ensure local temperature reading is valid
            //sendDataToPeer("192.168.1.150", localTemp); // Replace with actual peer IP
            sendDataToPeer("192.168.1.151", localTemp);
            sendDataToPeer("192.168.1.152", localTemp);
            sendDataToPeer("192.168.1.153", localTemp);
            sendDataToPeer("192.168.1.154", localTemp);
            sendDataToPeer("192.168.1.155", localTemp);
        }
    lastSendTime = currentTime;
  }
    
  if (autoMode) {
    manageTemperatureControl();
    
  }
  
  sendDataToServer();
  checkForAlerts();

  // Check if it's time to restart
  if (millis() - lastRestartTime >= restartInterval) {
    Serial.println("Restarting ESP32...");
    digitalWrite(LED, HIGH);
    digitalWrite(RELAY_1, HIGH);
    digitalWrite(RELAY_2, HIGH);
    //blinkFAST(9); 
    delay(10000); 
    ESP.restart();
  }
  
}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// FUNCTIONS //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void setupGPIO() {
  pinMode(RELAY_1, OUTPUT);
  digitalWrite(RELAY_1, HIGH);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_2, HIGH);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
}

void setupDHT() {
  dht.begin();
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  WiFi.config(staticIP, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  
  
}

void setupWebServer() {
  server.on("/dash", handleRoot);
  server.on("/toggleLamp", handleToggleLamp);
  server.on("/toggleFan", handleToggleFan);
  server.on("/setMode", handleSetMode);
  server.on("/setThreshold", handleSetThreshold);
  server.on("/data", sendSensorData);
  server.on("/receiveData", [](){
        if (server.hasArg("temp") && server.hasArg("sender")) {
            float temp = server.arg("temp").toFloat();
            String sender = server.arg("sender");
            processReceivedData(sender, temp);
            server.send(200, "text/plain", "Data Received");
        } else {
            server.send(404, "text/plain", "Data not found");
        }
    });
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Greenhouse Controller " + deviceName + " </h1>";
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
  html += "<script>setInterval(function() {fetch('/data').then(response => response.text()).then(data => document.getElementById('sensorData').innerHTML = data);}, 8000);</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleToggleLamp() {
  if (!autoMode) {
    relay1Status = !relay1Status;
    digitalWrite(RELAY_1, relay1Status ? LOW : HIGH);
  }
  server.sendHeader("Location", "/dash"); // Redirect to root to update the page immediately
  server.send(303); // HTTP response code for See Other, causing a redirect
}

void handleToggleFan() {
  if (!autoMode) {
    relay2Status = !relay2Status;
    digitalWrite(RELAY_2, relay2Status ? LOW : HIGH);
  }
  server.sendHeader("Location", "/dash"); // Redirect to root to update the page immediately
  server.send(303); // HTTP response code for See Other, causing a redirect
}

void handleSetMode() {
  if (server.hasArg("mode")) {
    autoMode = (server.arg("mode") == "auto");
  }
  server.sendHeader("Location", "/dash");
  server.send(303);
}

void handleSetThreshold() {
  if (server.hasArg("threshold")) {
    autoTempThreshold = server.arg("threshold").toFloat();
  }
  server.sendHeader("Location", "/dash");
  server.send(303);
}

void sendDataToServer() {
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastTime > sendData2Servers ) { //<<<<<<<<<<<<<<<<<<<<  X minutes interval
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    
    WiFiClient client;
    HTTPClient http;

    String serverPath = "http://192.168.1.88/Monitoring/GREEN/" + String(folder) +"/post.php?temperature=" + String(t) + "&humidity=" + String(h);

    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      //Serial.print("HTTP Response code: ");
      //Serial.println(httpResponseCode);
      //String payload = http.getString();
      //Serial.println(payload);
    }
    else {
      //Serial.print("Error code: ");
      //Serial.println(httpResponseCode);
    }
    http.end();

    lastTime = currentTime;
  }
}
void sendSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float averagePeerTemp = calculateAveragePeerTemperature();
  float effectiveTemp = calculateEffectiveTemperature(t, averagePeerTemp);

  
  String data = "<p>Temperature: " + String(t) + "°C</p>";
  data += "<p>Humidity: " + String(h) + "%</p>";
  data += "<p>Average Peer Temperature: " + String(averagePeerTemp) + "°C</p>";
  data += "<p>Effective Temperature: " + String(effectiveTemp) + "°C</p>";

  // Display each peer's temperature
  for (auto& temp : peerTemperatures) {
    data += "<p>" + temp.first + ": " + String(temp.second) + "°C</p>";
  }

  data += "<p>IR Lamp: " + String(relay1Status ? "ON" : "OFF") + "</p>";
  data += "<p>Fan: " + String(relay2Status ? "ON" : "OFF") + "</p>";
  
  data += "<p>Mode: " + String(autoMode ? "Automatic" : "Manual") + "</p>";
  data += "<p>Threshold: " + String(autoTempThreshold) + "°C</p>";
  server.send(200, "text/plain", data);
}

void blinkLED(int blinks) {
  for (int i = 0; i < blinks; i++) {
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
    delay(1000);
  }
}

void blinkFAST(int blinks) {
  for (int i = 0; i < blinks; i++) {
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
  }
}
void checkForAlerts() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    blinkFAST(3); // Blink X times
  }
  else if (t < lowTemp) {
    // Blink X times
    blinkLED(1); 
  }
}

void sendDataToPeer(const String& peerIP, float temperature) {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    String serverPath = "http://" + peerIP + "/receiveData?temp=" + temperature + "&sender=" + deviceName;

    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();

    if(httpResponseCode > 0){
        //Serial.print("Data sent to ");
        //Serial.print(peerIP);
        //Serial.println(". Response: " + String(httpResponseCode));
    }
    else {
        //Serial.print("Error sending data to ");
        //Serial.print(peerIP);
        //Serial.println(". Error: " + String(httpResponseCode));
    }

    http.end();
  }
}

void processReceivedData(const String& sender, float temp) {
    //Serial.print("Received data from ");
    //Serial.print(sender);
    //Serial.print(": Temperature = ");
    //Serial.println(temp);

    peerTemperatures[sender] = temp; // Update the map with the latest data

}

void manageTemperatureControl() {
  float localTemp = dht.readTemperature();
  bool localSensorOk = !isnan(localTemp);
  float averagePeerTemp = calculateAveragePeerTemperature();
  bool peerDataAvailable = !isnan(averagePeerTemp);
  float effectiveTemp = localTemp;

  if (localSensorOk && peerDataAvailable) {
    // Use the lower of the two temperatures
    effectiveTemp = min(localTemp, averagePeerTemp);
    
  } else if (peerDataAvailable) {
    effectiveTemp = averagePeerTemp;
    
  }

  // Check if effective temperature is valid before making decisions
  if (!isnan(effectiveTemp)) {
    controlRelaysBasedOnTemperature(effectiveTemp);
  } else {
    // Neither local nor peer data available, log error or take default action
   //blinkLED(5);
  }
}

float calculateEffectiveTemperature(float localTemp, float averagePeerTemp) {
  bool localSensorOk = !isnan(localTemp);
  bool peerDataAvailable = !isnan(averagePeerTemp);

  if (localSensorOk && peerDataAvailable) {
    return min(localTemp, averagePeerTemp);
  } else if (peerDataAvailable) {
    return averagePeerTemp;
  }
  return localTemp; // Default to local temperature if no peer data available
}

float calculateAveragePeerTemperature() {
  if (peerTemperatures.empty()) return NAN;

  float sum = 0;
  for (auto& temp : peerTemperatures) {
    sum += temp.second;
  }
  return sum / peerTemperatures.size();
}

void controlRelaysBasedOnTemperature(float temperature) {
  bool shouldTurnOn = temperature < autoTempThreshold;
  unsigned long currentMillis = millis();

  if (shouldTurnOn) {
    // Turn on lamp and record start time
    if (!lampOn && !fanOn) {
      lampOn = relay1Status = true;
      fanOn = relay2Status = false;
      lampStartTime = currentMillis;
      digitalWrite(RELAY_1, relay1Status ? LOW : HIGH);
    }

    // Check if lamp duration is over
    if (lampOn && currentMillis - lampStartTime >= lampDuration) {
      lampOn = relay1Status = false;
      digitalWrite(RELAY_1, relay1Status ? LOW : HIGH);
      // Start fan after lamp is off
      fanOn = relay2Status = true;
      fanStartTime = currentMillis;
      digitalWrite(RELAY_2, relay2Status ? LOW : HIGH);
    }

    // Turn off fan after its duration
    if (fanOn && currentMillis - fanStartTime >= fanDuration) {
      fanOn = relay2Status = false;
      digitalWrite(RELAY_2, relay2Status ? LOW : HIGH);
      delay(10);
    }
  } else {
    // Turn off both relay if temperature is above threshold
    digitalWrite(RELAY_1, relay1Status ? LOW : HIGH);
    digitalWrite(RELAY_2, relay2Status ? LOW : HIGH);
    lampOn = relay1Status = false;
    fanOn = relay2Status = false;
  }
  //server.sendHeader("Location", "/dash"); // Redirect to root to update the page immediately
  server.send(303); // HTTP response code for See Other, causing a redirect

}
