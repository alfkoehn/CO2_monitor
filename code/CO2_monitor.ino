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


// -------------------------------------------------------------------
// Some switches defining general behaviour of the program
// -------------------------------------------------------------------
#define WIFI_ENABLED false      // set to true if WiFi is desired, 
                                // otherwise corresponding code is not compiled
#define DEBUG true              // activate debugging
                                // true:  print info + data to serial monitor
                                // false: serial monitor is not used


// -------------------------------------------------------------------
// Import all required libraries
// -------------------------------------------------------------------
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

#define CO2_THRESHOLD1 600
#define CO2_THRESHOLD2 1000
#define CO2_THRESHOLD3 1500

#define WARNING_DIODE_PIN D8      // NodeMCU pin for red LED

#define MEASURE_INTERVAL 10       // seconds, minimum: 2 

#define SCREEN_WIDTH 128          // OLED display width in pixels
#define SCREEN_HEIGHT 32          // OLED display height in pixels

#define OLED_RESET LED_BUILTIN    // OLED reset pin, 4 is default
                                  // -1 if sharing Arduino reset pin
                                  // using NodeMCU, it is LED_BUILTIN
                                  
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SCD30 airSensor;
// use "unsigned long" for variables that hold time
//  --> value will quickly become too large for an int
unsigned long previousMilliseconds = 0;    // store last time scd30 was updated

// update scd30 readings every MEASURE_INTERVAL seconds
const long interval = MEASURE_INTERVAL*1000;

// switch to perform a forced recalibration
// should only be done once in a while and only when outside 
bool DO_FORCED_RECALIBRATION = false;

#if WIFI_ENABLED
  // temperature, humidity, CO2 for web-page, updated in loop()
  float temperature_web = 0.0;
  float humidity_web    = 0.0;
  float co2_web         = 0.0;

  // create AsyncWebServer object on port 80 (port 80 for http)
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

  x0  = 0;
  x1  = 84;

  display.clearDisplay();
  display.setCursor(x0,5);
  display.print("CO2 (ppm):");
  display.setCursor(x1,5);
  // for floats, 2nd parameter in display.print sets number of decimals
  display.print(co2, 0);
    
  display.setCursor(x0,15);
  display.print("temp. ( C)");
  display.setCursor(x0+7*6,15);
  display.cp437(true);  // enable full 256 char 'Code Page 437' font
  display.write(248);   // degree symbol
  display.setCursor(x1,15);
  display.print(temperature, 1);
    
  display.setCursor(x0,25);
  display.print("humidity (%):");
  display.setCursor(x1,25);
  display.print(humidity, 1);
    
  display.display();
}


void printEmoji( float value ) {
  // syntax for functions used to draw to OLED:
  // display.drawBitmap(x, y, bitmap data, bitmap width, bitmap height, color)
  // display.drawCircle(x, y, radius, color) 

  float start_angle,    // used for smiley mouth
        end_angle,      // used for smiley mouth
        i;              // used for smiley mouth
  int   smile_x0,       
        smile_y0,
        smile_r,
        emoji_r,
        emoji_x0,
        emoji_y0,
        eye_size;

  emoji_r   = SCREEN_HEIGHT/4;
  if (SCREEN_HEIGHT == 32) {
    emoji_x0  = SCREEN_WIDTH - (1*emoji_r+1);
    emoji_y0  = emoji_r*3-1;
    eye_size  = 1;
  } else if (SCREEN_HEIGHT == 64) {
    emoji_x0  = emoji_r;
    emoji_y0  = emoji_r*3-1;
    eye_size  = 2;
  }

  bool  plot_all;       // if set, plots all emojis (makes only sense for larger oled)

  plot_all  = false;
  if (int(value) == 0) {
    plot_all  = true;
  }

  if (value < CO2_THRESHOLD1){
    // very happy smiley face

    display.drawCircle(emoji_x0*1, emoji_y0, emoji_r, WHITE);

    start_angle = 20./180*PI; 
    end_angle   = 160./180*PI;
    smile_r   = emoji_r/2;
    smile_x0  = emoji_x0*1;
    smile_y0  = emoji_y0+emoji_r/6;
    for (i = start_angle; i < end_angle; i = i + 0.05) {
      display.drawPixel(smile_x0 + cos(i) * smile_r, smile_y0 + sin(i) * smile_r, WHITE);
    }

    display.drawLine(smile_x0+cos(start_angle)*smile_r, smile_y0+sin(start_angle)*smile_r,
                     smile_x0+cos(end_angle)*smile_r, smile_y0+sin(end_angle)*smile_r,
                     WHITE);
    
    // draw eyes
    display.fillCircle(emoji_x0*1-emoji_r/2/4*3, smile_y0-emoji_r/3, eye_size, WHITE);
    display.fillCircle(emoji_x0*1+emoji_r/2/4*3, smile_y0-emoji_r/3, eye_size, WHITE);
  }
  if ((value >= CO2_THRESHOLD1 && value < CO2_THRESHOLD2) || (plot_all == true)) {
    // happy smiley face

    if (SCREEN_HEIGHT == 32) {
      display.drawCircle(emoji_x0, emoji_y0, emoji_r, WHITE);
    } else if (SCREEN_HEIGHT == 64) {
      display.drawCircle(emoji_x0 + 2*emoji_r, emoji_y0, emoji_r, WHITE);
    }

    // draw mouth
    if (SCREEN_HEIGHT == 32) {
      smile_x0  = emoji_x0;
    } else if (SCREEN_HEIGHT == 64) {
      smile_x0  = emoji_x0 + 2*emoji_r;
    }
    start_angle = 20./180*PI; 
    end_angle   = 160./180*PI;
    smile_r   = emoji_r/2;
    smile_y0  = emoji_y0+emoji_r/6;
    for (i = start_angle; i < end_angle; i = i + 0.05) {
      display.drawPixel(smile_x0 + cos(i) * smile_r, smile_y0 + sin(i) * smile_r, WHITE);
    }

    // draw eyes
    display.fillCircle(smile_x0-emoji_r/2/4*3, smile_y0-emoji_r/3, eye_size, WHITE);
    display.fillCircle(smile_x0+emoji_r/2/4*3, smile_y0-emoji_r/3, eye_size, WHITE);
  }
  if ((value >= CO2_THRESHOLD2 && value < CO2_THRESHOLD3) || (plot_all == true)) {
    // not so happy smiley face

    if (SCREEN_HEIGHT == 32) {
      display.drawCircle(emoji_x0, emoji_y0, emoji_r, WHITE);
    } else if (SCREEN_HEIGHT == 64) {
      display.drawCircle(emoji_x0 + 4*emoji_r, emoji_y0, emoji_r, WHITE);
    }

    // draw mouth
    if (SCREEN_HEIGHT == 32) {
      smile_x0  = emoji_x0;
    } else if (SCREEN_HEIGHT == 64) {
      smile_x0  = emoji_x0 + 4*emoji_r;
    }
    display.drawLine(smile_x0-emoji_r/2/4*3, emoji_y0+emoji_r/2,
                     smile_x0+emoji_r/2/4*3, emoji_y0+emoji_r/2,
                     WHITE);

    // draw eyes
    display.fillCircle(smile_x0-emoji_r/2/4*3, smile_y0-emoji_r/3, eye_size, WHITE);
    display.fillCircle(smile_x0+emoji_r/2/4*3, smile_y0-emoji_r/3, eye_size, WHITE);
  }
  if ((value >= CO2_THRESHOLD3) || (plot_all == true)) {
    // sad smiley face

    if (SCREEN_HEIGHT == 32) {
      display.drawCircle(emoji_x0, emoji_y0, emoji_r, WHITE);
    } else if (SCREEN_HEIGHT == 64) {
      display.drawCircle(emoji_x0 + 6*emoji_r-1, emoji_y0, emoji_r, WHITE);
    }

    // draw mouth
    if (SCREEN_HEIGHT == 32) {
      smile_x0  = emoji_x0;
    } else if (SCREEN_HEIGHT == 64) {
      smile_x0  = emoji_x0 + 6*emoji_r;
    }
    start_angle = 200./180*PI; 
    end_angle   = 340./180*PI;
    smile_r   = emoji_r/2;
    smile_y0  = emoji_y0+emoji_r/6;
    for (i = start_angle; i < end_angle; i = i + 0.05) {
      display.drawPixel(smile_x0 + cos(i) * smile_r, smile_y0+emoji_r/2 + sin(i) * smile_r, WHITE);
    }

    // draw eyes
    display.fillCircle(smile_x0-emoji_r/2/4*3, smile_y0-emoji_r/3, eye_size, WHITE);
    display.fillCircle(smile_x0+emoji_r/2/4*3, smile_y0-emoji_r/3, eye_size, WHITE);
  }
  display.display();
}


void forced_recalibration(){
  // note: for best results, the sensor has to be run in a stable environment 
  //       in continuous mode at a measurement rate of 2s for at least two 
  //       minutes before applying the FRC command and sending the reference value
  // quoted from "Interface Description Sensirion SCD30 Sensor Module"
  
  String counter;

  int CO2_offset_calibration = 410;
  
  if (DEBUG == true){
    Serial.println("Starting to do a forced recalibration in 10 seconds");
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Warning:");
  display.println("forced recalibration");
  display.display();
  
  for (int ii=0; ii<10; ++ii){
    counter = String(10-ii);
    display.setCursor(ii*9,20);
    display.print(counter);
    display.display();
    delay(1000);
  }
  
  airSensor.setForcedRecalibrationFactor(CO2_offset_calibration);

  delay(1000);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Successfully recalibrated!");
  display.println("Only required ~once per year");
  display.display();
  delay(5000);
  
}


void airSensorSetup(){

  bool autoSelfCalibration = false;

  // altitude of place of operation in meters
  // Stuttgart: approx 300; Uni Stuttgart: approx 500
  int alt = 300;

  // Start sensor using the Wire port, but disable the auto-calibration
  if (airSensor.begin(Wire, autoSelfCalibration) == false) {
  //if (airSensor.begin() == false) {
    if (DEBUG == true)
      Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    while (1)
      ;
  }
  
  // SCD30 has data ready at maximum every two seconds
  // can be set to 1800 at maximum (30 minutes)
  //airSensor.setMeasurementInterval(2);

  // altitude compensation in meters
  // alternatively, one could also use:
  //   airSensor.setAmbientPressure(pressure_in_mBar)
  delay(1000);
  airSensor.setAltitudeCompensation(alt);
  
  
  float T_offset = airSensor.getTemperatureOffset();
  Serial.print("Current temp offset: ");
  Serial.print(T_offset, 2);
  Serial.println("C");

  // note that this value also depends on how you installed the SCD30
  airSensor.setTemperatureOffset(7); //5 is used in example (0 is default value)
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
  /* Explicitly set ESP8266 to be a WiFi-client, otherwise, it would, by
     default, try to act as both, client and access-point, and could cause
     network-issues with other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);     // connect to Wi-Fi
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

  // write previously defined emojis to display
  if (SCREEN_HEIGHT == 32) {
    printEmoji(400);
    delay(2000);
    printEmoji(600);
    delay(2000);
    printEmoji(1200);
    delay(2000);
    printEmoji(1800);
    delay(2000);
  } else {
    printEmoji(0);
  }
  
  display.display();              // write display buffer to display

  // turn warning LED on and off to test it
  digitalWrite(WARNING_DIODE_PIN, HIGH);
  delay(2000*2); 
  digitalWrite(WARNING_DIODE_PIN, LOW);

  // initialize SCD30
  airSensorSetup();

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

  // forced recalibration requires 2 minutes of stable environment in advance
  if ((DO_FORCED_RECALIBRATION == true) && (currentMilliseconds > 120000)) {
    forced_recalibration();
    DO_FORCED_RECALIBRATION = false;
  }
  
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
    if (co2_new >= CO2_THRESHOLD3) {
      digitalWrite(WARNING_DIODE_PIN, HIGH);
    } else {
      digitalWrite(WARNING_DIODE_PIN, LOW);
    }

    // print smiley with happiness according to CO2 concentration
    printEmoji( co2_new);
  }
}
