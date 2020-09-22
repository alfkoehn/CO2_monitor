/*
  
  A CO2 monitor displaying CO2 concentration on an OLED screen
  and to the serial monitor using SparkFun libraries.
  If the CO2 concentration is too high, a red LED is fired-up.

  In addition, the relative humidity in % and temperature in C is 
  also measured and displayed.
 
  Author:  Alf Köhn-Seemann
  Email:   alf.koehn@gmail.com
  License: MIT

  The following libraries need to installed manually:
    https://github.com/me-no-dev/ESPAsyncTCP
      click on "Code", select "Download ZIP"
    open Arduino IDE
      Sketch --> Include Libary --> Add .ZIP Library
      If previously already installed remove by deleting the corresponding folder in ~/Arduino/libraries/

    https://github.com/me-no-dev/ESPAsyncWebServer
      same as above

  All other libraries are available via the library manager.

  Hardware Connections:
  Attach NodeMCU to computer using a USB cable.
  Connect OLED display, SCD30, red LED to NodeMCU.
  Optional: Open Serial Monitor at 115200 baud.

*/

// Import required libraries
#include <Wire.h>                 // for I2C communication
#include <Adafruit_GFX.h>         // for writing to display
#include <Adafruit_SSD1306.h>     // for writing to display
#include "SparkFun_SCD30_Arduino_Library.h"

// set to true if WiFi is desired, otherwise corresponding code is not compiled
#define WIFI_ENABLED true

#if WIFI_ENABLED
  #include <ESP8266WiFi.h>
  #include <Hash.h>               // for SHA1 algorith (for Font Awesome)
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include "Webpageindex.h"       // webpage content, same folder as .ino file

  // Replace with your network credentials
  const char* ssid      = "ENTER_SSID";
  const char* password  = "ENTER_PASSWORD";
#endif

// activate debugging
//   true:  print info + data to serial monitor
//   false: serial monitor is not used
#define DEBUG true

#define CO2_CRITICAL 1500         // threshold for warning
#define WARNING_DIODE_PIN D8      // NodeMCU pin for red LED

#define MEASURE_INTERVAL 10       // seconds, minimum: 2 

#define SCREEN_WIDTH 128          // OLED display width in pixels
#define SCREEN_HEIGHT 32          // OLED display height in pixels

// OLDE reset pin, 4 is default (-1 if sharing Arduino reset pin)
// using NodeMCU, we have to use LED_BUILTIN
#define OLED_RESET LED_BUILTIN
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SCD30 airSensor;
// use "unsigned long" for variables that hold time
//  --> value will quickly become too large for an int
unsigned long previousMilliseconds = 0;    // store last time scd30 was updated

// update scd30 readings every MEASURE_INTERVAL seconds
const long interval = MEASURE_INTERVAL*1000;  

#if WIFI_ENABLED
  // temperature, humidity, CO2 for web-page, updated in loop()
  float temperature_web = 0.0;
  float humidity_web    = 0.0;
  float co2_web         = 0.0;

  // create AsyncWebServer object on port 80
  AsyncWebServer server(80);

  // read html into string
  String webpage = index_html;

  // function for replacing placeholder on webpage with SCD30 values
  String processor(const String& var){
    //Serial.println(var);
    if(var == "CO2"){
      return String(co2_web);
    }
    else if(var == "TEMPERATURE"){
      return String(temperature_web);
    }
    else if(var == "HUMIDITY"){
      return String(humidity_web);
    }
    return String();
  }
#endif


void printToSerial( float co2, float temperature, float humidity) {
  Serial.print("co2(ppm):");
  Serial.print(co2, 1);
  Serial.print(" temp(C):");
  Serial.print(temperature, 1);
  Serial.print(" humidity(%):");
  Serial.print(humidity, 1);
  Serial.println();
}


void printToOLED( float co2, float temperature, float humidity) {
  int
    x0, x1;           // to align output on OLED display vertically

  x0  = 2;
  x1  = 86;

  display.clearDisplay();
  display.setCursor(x0,5);
  display.print("CO2 (ppm):");
  display.setCursor(x1,5);
  // for floats, 2nd parameter in display.print sets number of decimals
  display.print(co2, 0);
    
  display.setCursor(x0,15);
  display.print("temp. ( C)");
  display.setCursor(x0+7*6,15);
  display.write(167);
  display.setCursor(x1,15);
  display.print(temperature, 1);
    
  display.setCursor(x0,25);
  display.print("humidity (%):");
  display.setCursor(x1,25);
  display.print(humidity, 1);
    
  display.display();
}


void setup(){
  if (DEBUG == true) {
    // initialize serial monitor at baud rate of 115200
    Serial.begin(115200);
    Serial.println("Using SCD30 to get: CO2 concentration, temperature, humidity");
  }

  // initialize I2C
  Wire.begin();

  // initialize LED pin as an output
  pinMode(WARNING_DIODE_PIN, OUTPUT);

#if WIFI_ENABLED
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  if (DEBUG == true)
    Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (DEBUG == true)
      Serial.print(".");
  }
  if (DEBUG == true)
    Serial.println(WiFi.localIP());
#endif

  // SSD1306_SWITCHCAPVCC: generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    if (DEBUG == true) 
      Serial.println(F("SSD1306 allocation failed"));
    for(;;);                      // don't proceed, loop forever
  }

  display.display();              // initialize display
                                  // library will show Adafruit logo
  delay(2000);                    // pause for 2 seconds
  display.clearDisplay();         // clear the buffer
  display.setTextSize(1);         // has to be set initially
  display.setTextColor(WHITE);    // has to be set initially

  // move cursor to position and print text there
  display.setCursor(2,5);
  display.println("CO2 monitor");
  display.println("twitter.com/formbar");
#if WIFI_ENABLED
  display.println(WiFi.localIP());
#else
  display.println("WiFi disabled");
#endif

  display.display();              // write display buffer to display

  // turn warning LED on and off to test it
  digitalWrite(WARNING_DIODE_PIN, HIGH);
  delay(2000); 
  digitalWrite(WARNING_DIODE_PIN, LOW);

  // initialize SCD30
  // SCD30 has data ready every two seconds
  if (airSensor.begin() == false) {
    if (DEBUG == true)
      Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    while (1)
      ;
  }

#if WIFI_ENABLED
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/co2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(co2_web).c_str());
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperature_web).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(humidity_web).c_str());
  });
  // Start server
  server.begin();
#endif
}
 
void loop(){

  float
    co2_new,
    temperature_new,
    humidity_new;
  
  unsigned long currentMilliseconds;

  // get milliseconds passed since program started to run
  currentMilliseconds = millis();
  
  if (currentMilliseconds - previousMilliseconds >= interval) {
    // save the last time you updated the DHT values
    previousMilliseconds = currentMilliseconds;

    if (airSensor.dataAvailable()) {
      // get data from SCD30 sensor
      co2_new         = airSensor.getCO2();
      temperature_new = airSensor.getTemperature();
      humidity_new    = airSensor.getHumidity();

      // print data to serial console
      if (DEBUG == true)
        printToSerial(co2_new, temperature_new, humidity_new);

      // print data to OLED display
      printToOLED(co2_new, temperature_new, humidity_new);

#if WIFI_ENABLED
      // updated values for webpage
      co2_web         = co2_new;
      temperature_web = temperature_new;
      humidity_web    = humidity_new;
#endif
    }

    // if CO2-value is too high, issue a warning  
    if (co2_new >= CO2_CRITICAL) {
      digitalWrite(WARNING_DIODE_PIN, HIGH);
    } else {
      digitalWrite(WARNING_DIODE_PIN, LOW);
    }
  }
}