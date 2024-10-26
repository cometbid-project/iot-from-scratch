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
//needed for library
#include "WiFiProv.h"
#include "WiFi.h"

/*******************************************************************************************************************
** Declare all program constants                                                                                  **
*******************************************************************************************************************/
#define SEALEVELPRESSURE_HPA (1013.25)

// #define USE_SOFT_AP // Uncomment if you want to enforce using Soft AP method instead of BLE

const char* pop = "abcd1234";           // Proof of possession - otherwise called a PIN - string provided by the device, entered by user in the phone app
const char* service_name = "PROV_123";  // Name of your device (the Espressif apps expects by default device name starting with "Prov_")
const char* service_key = NULL;         // Password used for SofAP method (NULL = no password needed)
bool reset_provisioned = true;          // When true the library will automatically delete previously provisioned data.
bool wifi_connected = 0;

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

void SysProvEvent(arduino_event_t* sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("\nConnected IP address : ");
      Serial.println(IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr));
      wifi_connected = 1;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("\nDisconnected. Connecting to the AP again... ");
      wifi_connected = 0;
      break;
    case ARDUINO_EVENT_PROV_START:
      Serial.println("\nProvisioning started\nGive Credentials of your access point using smartphone app");
      break;
    case ARDUINO_EVENT_PROV_CRED_RECV:
      {
        Serial.println("\nReceived Wi-Fi credentials");
        Serial.print("\tSSID : ");
        Serial.println((const char*)sys_event->event_info.prov_cred_recv.ssid);
        Serial.print("\tPassword : ");
        Serial.println((char const*)sys_event->event_info.prov_cred_recv.password);
        break;
      }
    case ARDUINO_EVENT_PROV_CRED_FAIL:
      {
        Serial.println("\nProvisioning failed!\nPlease reset to factory and retry provisioning\n");
        if (sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR)
          Serial.println("\nWi-Fi AP password incorrect");
        else
          Serial.println("\nWi-Fi AP not found....Add API \" nvs_flash_erase() \" before beginProvision()");
        break;
      }
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("\nProvisioning Successful");
      break;
    case ARDUINO_EVENT_PROV_END:
      Serial.println("\nProvisioning Ends");
      break;
    default:
      break;
  }
}

String processor(const String& var) {
  static int32_t temperature, humidity, pressure;         // Store readings
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

void connectToWifiProv() {
#if CONFIG_IDF_TARGET_ESP32 && CONFIG_BLUEDROID_ENABLED && not USE_SOFT_AP
  Serial.println("Begin Provisioning using BLE");
  // Sample uuid that user can pass during provisioning using BLE
  uint8_t uuid[16] = { 0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                       0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02 };
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name, service_key, uuid, reset_provisioned);
#else
  Serial.println("Begin Provisioning using Soft AP");
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name, service_key);
#endif

#if CONFIG_BLUEDROID_ENABLED && not USE_SOFT_AP
  log_d("ble qr");
  WiFiProv.printQR(service_name, pop, "ble");
#else
  log_d("wifi qr");
  WiFiProv.printQR(service_name, pop, "softap");
#endif
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);

  // Add list of Wifi networks
  //wifiMulti.addAP("ssid_wifi_name", "your_wifi_password");
  //wifiMulti.addAP("ssid_wifi_name", "your_wifi_password");

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
  /*
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected to");
    Serial.println(WiFi.SSID());
    Serial.println(WiFi.RSSI());
    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  }
  */
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

  handleWebServerEvents();

  ElegantOTA.begin(&server);  // Start ElegantOTA
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_SPEED);
  WiFi.onEvent(SysProvEvent);

  delay(100);

  Serial.println("Serial setup begins...");

  connectToWifiProv();
  //connectToWifi();
  Serial.println("Wifi Connection done...");
  connectToWebServer();

  setupBMESensor();
}

/*!
    @brief    Arduino method for the main program loop
    @details  This is the main program for the Arduino IDE, it is an infinite loop and keeps on repeating.
    @return   void
*/
void loop() {
  if (wifi_connected) {
    //Serial.println("Connected to WiFi");
  } else {
    Serial.println("Waiting For WiFi");
    delay(5000);
  }

  if ((millis() - lastTime) > timerDelay) {
    static int32_t temperature, humidity, pressure;         // Store readings
    BME280.getSensorData(temperature, humidity, pressure);  // Get most recent readings

    Serial.printf("Temperature = %.2f ºC \n", temperature / 100.0);
    Serial.printf("Humidity = %.2f % \n", humidity / 100.0);
    Serial.printf("Pressure = %.2f hPa \n", pressure / 100.0);
    Serial.printf("Altitude = %.2fm \n", altitude());
    Serial.println();

    // Send Events to the Web Server with the Sensor Readings
    events.send("ping", NULL, millis());
    events.send(String(temperature / 100.0).c_str(), "temperature", millis());
    events.send(String(humidity / 100.0).c_str(), "humidity", millis());
    events.send(String(pressure / 100.0).c_str(), "pressure", millis());
    events.send(String(altitude()).c_str(), "altitude", millis());
    // events.send(String(gasResistance).c_str(), "gas", millis());

    lastTime = millis();
  }

  if (digitalRead(0) == LOW) {
    Serial.println("Reset Button Pressed");
    delay(1000);

    if (digitalRead(0) == LOW) {
      Serial.println("Resetting");
      wifi_prov_mgr_reset_provisioning();
      ESP.restart();
    }
  }
}  // of method loop()
