/*
@section Adebowale Samuel Author

Written by Samuel\@Cometbid-project

@section I2CDemoversions Changelog
*/
#include <ArduinoOTA.h>
#include <BME280.h>  // Include the BME280 Sensor library
#include <ElegantOTA.h>
#include <ESPmDNS.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsync_WiFiManager.h>  //https://github.com/khoih-prog/ESPAsync_WiFiManager

WiFiMulti wifiMulti;

/*******************************************************************************************************************
** Declare all program constants                                                                                  **
*******************************************************************************************************************/
#define SEALEVELPRESSURE_HPA (1013.25)

const uint32_t SERIAL_SPEED = 115200;  ///< Default baud rate for Serial I/O

const char* host = "simple_iot";

/*******************************************************************************************************************
** Declare global variables and instantiate classes                                                               **
*******************************************************************************************************************/
BME280_Class BME280;  ///< Create an instance of the BME280 class

AsyncWebServer server(80);
AsyncEventSource events("/events");

// Current time
// unsigned long currentTime = millis();
// Previous time
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;  // send readings timer

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>BME280 Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    .card.pressure { color: #3fca6b; }
    .card.altitude { color: #d62246; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>BME280 Weather Station</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4 style="color: Blue;"><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p style="color: Blue;"><span class="reading"><span id="temp">%TEMPERATURE%</span> &deg;C</span></p>
        <h4 style="color: Green;"><i class="fas fa-tint"></i> HUMIDITY</h4><p style="color: Green;"><span class="reading"><span id="hum">%HUMIDITY%</span> &percnt;</span></p>
        <h4 style="color: Orange;"><i class="fas fa-angle-double-down"></i> PRESSURE</h4><p style="color: Orange;"><span class="reading"><span id="pres">%PRESSURE%</span> hPa</span></p>
        <h4 style="color: Red;"><i class="fas fa-wind"></i> ALTITUDE</h4><p style="color: Red;"><span class="reading"><span id="alti">%ALTITUDE%</span> m</span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');

 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);

 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);

 source.addEventListener('temperature', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp").innerHTML = e.data;
 }, false);

 source.addEventListener('humidity', function(e) {
  console.log("humidity", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);

 source.addEventListener('pressure', function(e) {
  console.log("pressure", e.data);
  document.getElementById("pres").innerHTML = e.data;
 }, false);

 source.addEventListener('altitude', function(e) {
  console.log("altitude", e.data);
  document.getElementById("alti").innerHTML = e.data;
 }, false);
}
</script>
</body>
</html>)rawliteral";


/*!
* @brief     This converts a pressure measurement into a height in meters
* @details   The corrected sea-level pressure can be passed into the function if it is known, otherwise the standard
*            atmospheric pressure of 1013.25hPa is used (see https://en.wikipedia.org/wiki/Atmospheric_pressure
* @param[in] seaLevel Sea-Level pressure in millibars
* @return    floating point altitude in meters.
*/
float altitude(const float seaLevel = 1013.25) {
  static float Altitude;
  int32_t temp, hum, press;
  BME280.getSensorData(temp, hum, press);  // Get the most recent values from the device

  Altitude = 44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903));  // Convert into altitude in meters
  return (Altitude);
}  // of method altitude()


String processor(const String& var) {
  static int32_t temperature, humidity, pressure;  // Store readings
  BME280.getSensorData(temperature, humidity, pressure);  // Get most recent readings

  //Serial.println(var);
  if (var == "TEMPERATURE") {
    return String(temperature / 100.0);
  } else if (var == "HUMIDITY") {
    return String(humidity / 100.0);
  } else if (var == "PRESSURE") {
    return String(pressure / 100.0);
  } /*
  else if (var == "GAS") {
    return String(gasResistance);
  } */
  else if (var == "ALTITUDE") {
    return String(altitude());
  }
}

void handleWebServerEvents() {
  // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient* client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  server.begin();
  Serial.println("HTTP server started");
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);

  // Add list of Wifi networks
  wifiMulti.addAP("your-wifi-name", "your-wifi-password");
  wifiMulti.addAP("another-wifi-name", "another-wifi-password");
  //wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  // WiFi.scanNetworks will returnthe number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("No network found");
  } else {
    Serial.print(n);
    Serial.println(" network found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      Serial.println("");

      delay(10);
    }
  }

  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.println(WiFi.RSSI());
    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  }
}

void setupBMESensor() {
#ifdef __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait 3 seconds to initialize USB
  delay(3000);
#endif

  Serial.println(F("Starting I2CDemo example program for BME280"));
  Serial.print(F("- Initializing BME280 sensor\n"));

  while (!BME280.begin(I2C_FAST_MODE_PLUS_MODE))  // Start BME280 using I2C protocol
  {
    Serial.println(F("-  Unable to find BME280. Waiting 3 seconds."));
    delay(3000);
  }  // of loop until device is located

  BME280.mode(SleepMode);
  Serial.print(F("- Sensor detected in operating mode \""));
  Serial.print(BME280.mode());
  Serial.println(F("\"."));

  if (BME280.mode() == 0) {
    Serial.print(F("- Turning sensor to normal mode, mode is now \""));
    Serial.print(BME280.mode(NormalMode));  // Use enumerated type values
    Serial.println("\"");
  }  // of if-then we have sleep mode

  Serial.println(F("- Setting 16x oversampling for all sensors"));
  BME280.setOversampling(TemperatureSensor, Oversample16);
  BME280.setOversampling(HumiditySensor, Oversample16);
  BME280.setOversampling(PressureSensor, Oversample16);
  Serial.println(F("- Setting IIR filter to maximum value of 16 samples"));

  BME280.iirFilter(IIR16);
  Serial.println(F("- Setting time between measurements to 1 second"));

  BME280.inactiveTime(inactive1000ms);
  Serial.print(F("- Each measurement cycle will take "));
  Serial.print(BME280.measurementTime(MaximumMeasure) / 1000);
  Serial.println(F("ms.\n\n"));
}

void connectToWebServer() {
  /* use mdns for host name resolution */
  if (!MDNS.begin(host)) {  //http://simple_iot.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started, connect using : Station IP-Address or http://simple_iot.local");

  ElegantOTA.begin(&server);  // Start ElegantOTA
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_SPEED);
  delay(10);

  Serial.println("Serial setup begins...");

  connectToWifi();
  Serial.println("Wifi Connection done...");
  connectToWebServer();

  setupBMESensor();
  handleWebServerEvents();
}

/*!
    @brief    Arduino method for the main program loop
    @details  This is the main program for the Arduino IDE, it is an infinite loop and keeps on repeating.
    @return   void
*/
void loop() {
  if ((millis() - lastTime) > timerDelay) {
    static int32_t temperature, humidity, pressure;         // Store readings
    BME280.getSensorData(temperature, humidity, pressure);  // Get most recent readings

    Serial.println();
    Serial.printf("Temperature = %.2f ºC \n", temperature / 100.0);
    Serial.printf("Humidity = %.2f % \n", humidity / 100.0);
    Serial.printf("Pressure = %.2f hPa \n", pressure / 100.0);
    Serial.printf("Altitude = %.2fm \n", altitude());
    Serial.println();

    // Send Events to the Web Server with the Sensor Readings
    events.send("ping", NULL, millis());
    events.send(String(temperature).c_str(), "temperature", millis());
    events.send(String(humidity).c_str(), "humidity", millis());
    events.send(String(pressure).c_str(), "pressure", millis());
    events.send(String(altitude()).c_str(), "altitude", millis());
    // events.send(String(gasResistance).c_str(), "gas", millis());

    lastTime = millis();
  }
}  // of method loop()
