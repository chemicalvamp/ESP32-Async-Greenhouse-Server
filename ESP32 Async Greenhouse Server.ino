/*
  Rui Santos guides https://RandomNerdTutorials.com/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#define DHTTYPE DHT11
#define ESPALEXA_ASYNC //it is important to define this before #include <Espalexa.h>!
#include <Espalexa.h>

#ifdef AVR
#include <Servo.h>
#include <wire.h>
#elif defined(ESP32)
#include <ESP32Servo.h>
#endif

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include "time.h"
#include <ESPAsyncWebServer.h>

// Instanciate servos
Servo servo1;
Servo servo2;

// Constants
const int SERVO1PIN = 14;
const int SERVO2PIN = 15;
const int LEDPIN = 16;
const int DHTPIN = 22;
const int LIGHT1PIN = 26;
const int LIGHT2PIN = 27;
const int SOILPIN = 32;
const int LDRPIN = 36;

const char* MACAddress = "24:6f:28:b0:15:50";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -21600; // -6 hours CST
const int   daylightOffset_sec = 0;

// Variables
float temperature = 30;
float humidity = 40;
float heatindex = 50;
float lightlevel = 60;
int waterlevel = 70;

// prototypes
boolean connectWifi();
void printLocalTime();

//callback functions
void ledChanged(uint8_t brightness);
void firstLightChanged(uint8_t brightness);
void secondLightChanged(uint8_t brightness);
void firstServoChanged(uint8_t brightness);
void secondServoChanged(uint8_t brightness);

// device names
String Device_0_Name = "LED";
String Device_1_Name = "Servo1";
String Device_2_Name = "Servo2";
String Device_3_Name = "Light1";
String Device_4_Name = "Light2";

// Replace with your network credentials
const char* ssid = "SpectrumSetup-48";
const char* password = "wittyberry493"; // 493
const char* APssid = "Greenhouse_Down";
const char* APpassword = "password";
boolean wifiConnected = false;

// Starts
DHT dht(DHTPIN, DHTTYPE);

// Webstuff
Espalexa espalexa;
AsyncWebServer server(80);

String readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius () or (false)
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(t);
	  temperature = t;
    return String(t);
  }
}

String readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(h);
	  humidity = h;
    return String(h);
  }
}

String calculateHeatIndex() {
  // Math
  float i = dht.computeHeatIndex(temperature, humidity, false);
  if (isnan(i)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else {
    Serial.println(i);
	  heatindex = i;
    return String(i);
  }
}

String readLightLevel() {
  // Sensor
  int l = analogRead(LDRPIN);
  if (isnan(l)) {
    Serial.println("Failed to read from LDR!");
    return "--";
  }
  else {
    Serial.println(l);
	  lightlevel = l;
    return String(l);
  }
}

String readWaterLevel() {
  // Sensor
  int w = analogRead(SOILPIN);
  if (isnan(w)) {
    Serial.println("Failed to read from Capacitive moisture sensor!");
    return "--";
  }
  else {
    Serial.println(w);
	  waterlevel = w;
    return String(w);
  }
}
//fa-thermometer-empty -half -full
//fa-solid fa-island-tropical
// blue 0x green 0x red 0x
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.15.4/css/all.css" integrity="sha384-DyZ88mC6Up2uqS4h/KRgHuoeGwBcD4Ng9SiP4dIRy0EXTlnuz47vAwmeGwVChigm" crossorigin="anonymous"/>
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP32 Async Greenhouse Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i>
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>
  <p>
    <i class="fas fa-fire-alt" style="color:#ff2a04;"></i> 
    <span class="dht-labels">HeatIndex</span>
    <span id="heatindex">%HEATINDEX%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-cloud-sun" style="color:#ffd966;"></i> 
    <span class="dht-labels">LightLevel</span>
    <span id="lightlevel">%LIGHTLEVEL%</span>
    <sup class="units">Lux</sup>
  </p>
  <p>
    <i class="fas fa-fill-drip" style="color:#6fa8dc;"></i> 
    <span class="dht-labels">WaterLevel</span>
    <span id="waterlevel">%WATERLEVEL%</span>
    <sup class="units">High(3000)Dry</sup>
  </p>
</body>
<script>
  
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperature").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
  }, 10000 ) ;
  
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("humidity").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();
  }, 10000 ) ;
  
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("heatindex").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/heatindex", true);
    xhttp.send();
  }, 10000 ) ;
  
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("lightlevel").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/lightlevel", true);
    xhttp.send();
  }, 10000 ) ;
  
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("waterlevel").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/waterlevel", true);
    xhttp.send();
  }, 10000 ) ;
  
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var) {
  if(var == "TEMPERATURE") {
    return readDHTTemperature();
  }
  else if(var == "HUMIDITY") {
    return readDHTHumidity();
  }
  else if(var == "HEATINDEX") {
    return calculateHeatIndex();
  }
  else if(var == "LIGHTLEVEL") {
    return readLightLevel();
  }
  else if(var == "WATERLEVEL") {
    return readWaterLevel();
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  
  // Set modes
  pinMode(LDRPIN, INPUT);
  pinMode(LEDPIN, OUTPUT);
  pinMode(SERVO1PIN, OUTPUT);
  pinMode(SERVO2PIN, OUTPUT);
  pinMode(LIGHT1PIN, OUTPUT);
  pinMode(LIGHT2PIN, OUTPUT);
  
  // Attach the code to servo
  servo1.attach(SERVO1PIN);
  servo2.attach(SERVO2PIN);
  
  // DHT Heat index sensor
  dht.begin();
  
  // Set default outputs
  digitalWrite(LEDPIN, HIGH);
  digitalWrite(LIGHT1PIN, LOW);
  digitalWrite(LIGHT2PIN, LOW);
  
  // Initialise wifi connection
  wifiConnected = connectWifi();

  if (wifiConnected) {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, processor);
    });
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", readDHTTemperature().c_str());
    });
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", readDHTHumidity().c_str());
    });
    server.on("/heatindex", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", calculateHeatIndex().c_str());
    });
    server.on("/lightlevel", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", readLightLevel().c_str());
    });
    server.on("/waterlevel", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", readWaterLevel().c_str());
    });
    
    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();
	
    // Define your devices here.
    espalexa.addDevice(Device_0_Name, ledChanged);			// LED
    espalexa.addDevice(Device_1_Name, firstServoChanged); 	// Servo1
    espalexa.addDevice(Device_2_Name, secondServoChanged); 	// Servo2
    espalexa.addDevice(Device_3_Name, firstLightChanged); 	// Light1
    espalexa.addDevice(Device_4_Name, secondLightChanged); 	// Light2
    
    // Start server
    
    espalexa.begin(&server); //give espalexa a pointer to your server object so it can use your server instead of creating its own
    //server.begin(); //omit this since it will be done by espalexa.begin(&server)
  }
  else
  {
      Serial.println("Cannot connect to WiFi. Please check data and reset the ESP.");
  }
}

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y");
  Serial.println(&timeinfo, "%H:%M:%S");
  /*
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
  */
}

//our callback functions
void ledChanged(uint8_t brightness)
{
  //Control the device TODO: PWM
  if (brightness == 255)
  {
	digitalWrite(LEDPIN, HIGH);
    delay(100);
    Serial.println("LED ON");
  }
  else
  {
	digitalWrite(LEDPIN, LOW);
    delay(100);
    Serial.println("LED OFF");
  }
}
void firstLightChanged(uint8_t brightness)
{
  //Control the device TODO: PWM
  if (brightness == 255)
  {
	digitalWrite(LIGHT1PIN, HIGH);
    delay(100);
    Serial.println("Light1 ON");
  }
  else
  {
	digitalWrite(LIGHT1PIN, LOW);
    delay(100);
    Serial.println("Light1 OFF");
  }
}
void secondLightChanged(uint8_t brightness)
{
  //Control the device TODO: PWM
  if (brightness == 255)
  {
	digitalWrite(LIGHT2PIN, HIGH);
    delay(100);
    Serial.println("Light2 ON");
  }
  else
  {
	digitalWrite(LIGHT2PIN, LOW);
    delay(100);
    Serial.println("Light2 OFF");
  }
}
void firstServoChanged(uint8_t brightness)
{
  //Control the device
  if (brightness == 255)
    {

      servo1.write(0);
      delay(1000);
      
      Serial.println("Servo1 255");
    }
  else
  {
   servo1.write(100);
    delay(1000);
    Serial.println("Servo1 100");
  }
}
void secondServoChanged(uint8_t brightness)
{
  //Control the device
  if (brightness == 255)
    {

      servo2.write(0);
      delay(1000);
      
      Serial.println("Servo2 255");
    }
  else
  {
   servo2.write(100);
    delay(1000);
    Serial.println("Servo2 100");
  }
}

// connect to wifi – returns true if successful or false if not
boolean connectWifi()
{
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
    Serial.println("Attempting AP mode.");
    WiFi.softAP(APssid, APpassword);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    return true;
  }
  return state;
}

void loop() {
   espalexa.loop();
   delay(100);
}