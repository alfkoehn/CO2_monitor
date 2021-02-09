/*
  
  A CO2 monitor displaying CO2 concentration on an OLED or LCD screen
  and to the serial monitor using SparkFun libraries.
  If the CO2 concentration is too high, a warning will be displayed:
  depending on your set-up, this might be a red LED or something on the screen.

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
  Connect OLED or LCD display, SCD30, red LED to NodeMCU.
  Optional: Open Serial Monitor at 115200 baud.

*/


// -------------------------------------------------------------------
// Some switches defining general behaviour of the program
// -------------------------------------------------------------------
// moved to settings.h
#include "settings.h"
#include "credentials.h"  


// -------------------------------------------------------------------


// -------------------------------------------------------------------
// Import all required libraries
// -------------------------------------------------------------------
#include <Wire.h>                 // for I2C communication
#ifdef DISPLAY_OLED
  #include <Adafruit_GFX.h>       // for writing to display
  #include <Adafruit_SSD1306.h>   // for writing to display
#endif
#ifdef DISPLAY_LCD
  #include <LiquidCrystal_I2C.h>
#endif
#include <SparkFun_SCD30_Arduino_Library.h>

#include <ESP8266WiFi.h>          // also allows to explicitely turn WiFi off
#if WIFI_ENABLED
  #if WIFI_WEBSERVER
    #include <Hash.h>             // for SHA1 algorith (for Font Awesome)
    #include <ESPAsyncTCP.h>
    #include <ESPAsyncWebServer.h>

    #include "Webpageindex.h"     // webpage content, same folder as .ino file
  #endif

  #if WIFI_MQTT
    #include <PubSubClient.h>     // allows to send and receive MQTT messages

    //add local MQTT server IP here. Or in config.h
    const char* mqttserver    = MQTT_SERVER_IP;
    const char* mqtt_user     = MQTT_USERNAME; 
    const char* mqtt_passwd   = MQTT_PASSWORD;
    const char* mqtt_topic_prefix    = MQTT_TOPIC_PREFIX;
  #endif
  // Replace with your network credentials. Or in config.h
  const char* ssid      = WIFI_SSID;
  const char* password  = WIFI_PASSWORD;
  const char* deviceName = ESP_DEVICE_NAME;
#endif
// -------------------------------------------------------------------



const int lcdColumns  = 20;       // LCD: number of columns
const int lcdRows     = 4;        // LCD: number of rows

const float TempOffset =  TEMP_OFFSET; 

const int altitudeOffset = ALTITUDE_OFFSET;   // altitude of place of operation in meters
                                  // Stuttgart: approx 300; Uni Stuttgart: approx 500; Lohne: 67

// update scd30 readings every MEASURE_INTERVAL seconds
const long interval = MEASURE_INTERVAL*1000;

// use "unsigned long" for variables that hold time
//  --> value will quickly become too large for an int
// store last time scd30 was updated
unsigned long previousMilliseconds = 0;    

// switch to perform a forced recalibration
// should only be done once in a while and only when outside 
bool DO_FORCED_RECALIBRATION = false;
// -------------------------------------------------------------------


// -------------------------------------------------------------------
// Function prototypes (not needed in Arduino, but good practice imho)
// -------------------------------------------------------------------
void airSensorSetup();
void forced_recalibration();
void printToSerial( float co2, float temperature, float humidity);
#ifdef DISPLAY_OLED
  void printToOLED( float co2, float temperature, float humidity);
  void printEmoji( float value );
#endif
#ifdef DISPLAY_LCD
  void printToLCD( float co2, float temperature, float humidity);
  void scrollLCDText( int row, String message, int delayTime );
#endif
// -------------------------------------------------------------------


// -------------------------------------------------------------------
// Hardware declarations
// -------------------------------------------------------------------
#ifdef DISPLAY_OLED                                  
  // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif
#ifdef DISPLAY_LCD
  // run I2C scanner if LCD address is unknown
  LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
#endif

SCD30 airSensor;
// -------------------------------------------------------------------


#if WIFI_ENABLED
  // temperature, humidity, CO2 for web-page, updated in loop()
  float temperature_web = 0.0;
  float humidity_web    = 0.0;
  float co2_web         = 0.0;

  #if WIFI_WEBSERVER
    // create AsyncWebServer object on port 80 (port 80 for http)
    AsyncWebServer server(80);
  #endif
  #if WIFI_MQTT
    // declare (initialize) object of class WiFiClient (to establish connection to IP/port)
    WiFiClient espClient;
    // declare (initialize) object of class PubSubClient
    // input: constructor of previously defined WiFiClient
    PubSubClient mqttClient(espClient);
    // message to be published to mqtt topic
    char mqttMessage[10];
    char mqttTopic[28];
  #endif
#endif

#if SEND_VCC
  ADC_MODE(ADC_VCC);
  int vdd;
#endif

#ifdef WIFI_MQTT

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  sprintf(mqttTopic, "%s%s", mqtt_topic_prefix, "forceCal");
  if (strcmp(topic, mqttTopic) == 0) {
    
    if ((char)payload[0] == '1') {
       Serial.println("Forced recalibration via MQTT");
       DO_FORCED_RECALIBRATION = true;
    } 
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    //String clientId = "ESP8266Client-";
    //clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(deviceName, mqtt_user, mqtt_passwd)) { // if (client.connect(clientId.c_str(),"mspn","hau-den-krukkel1")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // ... and resubscribe
      sprintf(mqttTopic, "%s%s", mqtt_topic_prefix, "forceCal");
      mqttClient.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

#endif

void setup(){
  if (DEBUG == true) {
    // initialize serial monitor at baud rate of 115200
    Serial.begin(115200);
    delay(1000);
    Serial.println("Using SCD30 to get: CO2 concentration, temperature, humidity");
  }

  // initialize I2C
  Wire.begin();

#ifdef WARNING_DIODE_PIN
  // initialize LED pin as an output
  pinMode(WARNING_DIODE_PIN, OUTPUT);
#endif

#if WIFI_ENABLED
  #if WIFI_MQTT
  // configure mqtt server details, after that client is ready
  // create connection to mqtt broker
  mqttClient.setServer(mqttserver, 1883);
  mqttClient.setCallback(callback);
  #endif
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
  IPAddress ip = WiFi.localIP();
  if (DEBUG == true)
    Serial.println(ip);

  #if WIFI_WEBSERVER
    // -------------------------------------------------------------------
    // This is executed when you open the IP in browser
    // -------------------------------------------------------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      // note: do NOT load MAIN_page into a String variable
      //       this might not work (probably too large)
      request->send_P(200, "text/html", MAIN_page );
    });

    // this page is called by java Script AJAX
    server.on("/readData", HTTP_GET, [](AsyncWebServerRequest *request) {
      // putting all values into one big string
      // inspiration: https://circuits4you.com/2019/01/11/nodemcu-esp8266-arduino-json-parsing-example/
      String data2send = "{\"COO\":\""+String(co2_web)
                         +"\", \"Temperature\":\""+String(temperature_web) 
                         +"\", \"Humidity\":\""+ String(humidity_web) +"\"}";
      request->send_P(200, "text/plain", data2send.c_str());
    });
    // -------------------------------------------------------------------
  
    server.begin();
  #endif
#else
  WiFi.mode( WIFI_OFF );          // explicitely turn WiFi off
  WiFi.forceSleepBegin();         // explicitely turn WiFi off
  delay( 1 );                     // required to apply WiFi changes
  if (DEBUG == true) 
    Serial.println("WiFi is turned off.");
#endif

#ifdef DISPLAY_OLED
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
    display.println(ip);
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
#endif

#ifdef DISPLAY_LCD
  lcd.init();                     // initialize LCD
  lcd.backlight();                // turn on LCD backlight
  lcd.setCursor(0,0);             // set cursor to (column,row)
  lcd.print("WiFi connected");
  lcd.setCursor(0,1);
  lcd.print(ip);
  delay(2000);
#endif

#ifdef WARNING_DIODE_PIN
  // turn warning LED on and off to test it
  digitalWrite(WARNING_DIODE_PIN, HIGH);
  delay(2000*2); 
  digitalWrite(WARNING_DIODE_PIN, LOW);
#endif

  // initialize SCD30
  airSensorSetup();
}


void loop(){

  float
    co2_new,
    temperature_new,
    humidity_new;
  
  unsigned long currentMilliseconds;

#if WIFI_MQTT
  if (!mqttClient.connected()) {
    reconnect();          // from PubSubClient Example
  }                       // Establish MQTT connection
  mqttClient.loop();
#endif
  // get milliseconds passed since program started to run
  currentMilliseconds = millis();

  // forced recalibration requires 2 minutes of stable environment in advance
  if ((DO_FORCED_RECALIBRATION == true) && (currentMilliseconds > 120000)) {
    forced_recalibration();
    // Recalibration success? Blink twice ;-)
    digitalWrite(WARNING_DIODE_PIN, HIGH);
    delay(150);
    digitalWrite(WARNING_DIODE_PIN, LOW);
    delay(50);
    digitalWrite(WARNING_DIODE_PIN, HIGH);
    delay(150);
    digitalWrite(WARNING_DIODE_PIN, LOW);
    DO_FORCED_RECALIBRATION = false;
  }
  
  if (currentMilliseconds - previousMilliseconds >= interval) {
    // save the last time you updated the DHT values
    previousMilliseconds = currentMilliseconds;
#if SEND_VCC
    vdd = ESP.getVcc();
#endif

    if (airSensor.dataAvailable()) {
      // get updated data from SCD30 sensor
      co2_new         = airSensor.getCO2();
      temperature_new = airSensor.getTemperature();
      humidity_new    = airSensor.getHumidity();

      // print data to serial console
      if (DEBUG == true)
        printToSerial(co2_new, temperature_new, humidity_new);

      // print data to display
#ifdef DISPLAY_OLED
      printToOLED(co2_new, temperature_new, humidity_new);
      // print smiley with happiness according to CO2 concentration
      printEmoji( co2_new);
#endif
#ifdef DISPLAY_LCD
      printToLCD(co2_new, temperature_new, humidity_new);
      if (co2_new > CO2_THRESHOLD3)
        scrollLCDText( 3, "LUEFTEN", 250 );
#endif
      // if CO2-value is too high, issue a warning  
      if (co2_new >= CO2_THRESHOLD3) {
#ifdef WARNING_DIODE_PIN
        digitalWrite(WARNING_DIODE_PIN, HIGH);
#endif
      } else {
#ifdef WARNING_DIODE_PIN
        digitalWrite(WARNING_DIODE_PIN, LOW);
#endif
      }
#if WIFI_ENABLED
      // updated values for webpage
      co2_web         = co2_new;
      temperature_web = temperature_new;
      humidity_web    = humidity_new;
#if WIFI_MQTT
      // boolean connect (clientID, [username, password])
      // see https://pubsubclient.knolleary.net/api
      //mqttClient.connect(deviceName, mqtt_user, mqtt_passwd);
      //sprintf(mqttTopic, "%s%s", mqtt_topic_prefix, "forceCal");
      //mqttClient.subscribe(mqttTopic);
#if SEND_VCC
      sprintf(mqttMessage, "%d", vdd);
      // boolean publish (topic, payload)
      // publish message to the specified topic
      sprintf(mqttTopic, "%s%s", mqtt_topic_prefix, "vcc");
      mqttClient.publish(mqttTopic, mqttMessage );
#endif
      sprintf(mqttTopic, "%s%s", mqtt_topic_prefix, "hostname");
      mqttClient.publish(mqttTopic, deviceName );
      sprintf(mqttTopic, "%s%s", mqtt_topic_prefix, "co2");
      sprintf(mqttMessage, "%6.2f", co2_web);
      mqttClient.publish(mqttTopic, mqttMessage );
      sprintf(mqttTopic, "%s%s", mqtt_topic_prefix, "temp");
      sprintf(mqttMessage, "%6.2f", temperature_web);
      mqttClient.publish(mqttTopic, mqttMessage );
      sprintf(mqttTopic, "%s%s", mqtt_topic_prefix, "hum");
      sprintf(mqttMessage, "%6.2f", humidity_web);
      mqttClient.publish(mqttTopic, mqttMessage );
#endif
#endif
    } else {  // If sensor is not ready, blink twice ;-)
      Serial.println("Sensor not ready...");
      digitalWrite(WARNING_DIODE_PIN, HIGH);
      delay(150);
      digitalWrite(WARNING_DIODE_PIN, LOW);
      delay(50);
      digitalWrite(WARNING_DIODE_PIN, HIGH);
      delay(150);
      digitalWrite(WARNING_DIODE_PIN, LOW);
    }
  }
#ifndef WIFI_MQTT
  delay(100); // MQTT uses mqttClient.loop; a delay is not needed
#endif
}


// -------------------------------------------------------------------
// Function declarations
// -------------------------------------------------------------------
void airSensorSetup(){

  bool autoSelfCalibration = false;

  // start sensor using the Wire port
  if (airSensor.begin(Wire) == false) {
    if (DEBUG == true)
      Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    while (1)
      ;
  }

  // disable auto-calibration (import, see full documentation for details)
  airSensor.setAutoSelfCalibration(autoSelfCalibration);
  
  // SCD30 has data ready at maximum every two seconds
  // can be set to 1800 at maximum (30 minutes)
  //airSensor.setMeasurementInterval(MEASURE_INTERVAL);

  // altitude compensation in meters
  // alternatively, one could also use:
  //   airSensor.setAmbientPressure(pressure_in_mBar)
  delay(1000);
  airSensor.setAltitudeCompensation(altitudeOffset);
  
  float T_offset = airSensor.getTemperatureOffset();
  Serial.print("Current temp offset: ");
  Serial.print(T_offset, 2);
  Serial.println("C");

  // note: this value also depends on how you installed 
  //       the SCD30 in your device 
  airSensor.setTemperatureOffset(TempOffset);
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

#ifdef DISPLAY_OLED
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
#endif
  
  airSensor.setForcedRecalibrationFactor(CO2_offset_calibration);

#ifdef DISPLAY_OLED
  delay(1000);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Successfully recalibrated!");
  display.println("Only required ~once per year");
  display.display();
#endif
  
  delay(5000);
}


void printToSerial( float co2, float temperature, float humidity) {
  Serial.print("co2(ppm):");
  Serial.print(co2, 1);
  Serial.print(" temp(C):");
  Serial.print(temperature, 1);
  Serial.print(" humidity(%):");
  Serial.print(humidity, 1);
  Serial.println();
}


#ifdef DISPLAY_OLED
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
#endif


#ifdef DISPLAY_LCD
void printToLCD( float co2, float temperature, float humidity) {

  byte degreeSymbol[8] = {
    0b01100, 0b10010, 0b10010, 0b01100,
    0b00000, 0b00000, 0b00000, 0b00000
  };
  // allocate custom char to a location
  lcd.createChar(0, degreeSymbol);

  //int waitTime  = 2000;
  lcd.clear();
  //DrawYoutube();
  //delay(waitTime);

  // print co2 concentration (1st line, i.e. row 0)
  lcd.setCursor(0,0);
  lcd.print("CO2 in ppm");
  // make output right-aligned
  lcd.setCursor( (lcdColumns - (int(log10(co2))+1)), 0);
  lcd.print(int(round(co2)));

  // print temperature (2nd line, i.e. row 1)
  lcd.setCursor(0,1);
  lcd.print("Temp. in ");
  lcd.write(0);
  lcd.print("C");
  // make output right-aligned
  lcd.setCursor( (lcdColumns - (int(log10(temperature))+1)), 1);
  lcd.print(int(round(temperature)));

  // print humidity (3nd line, i.e. row 2)
  lcd.setCursor(0,2);
  lcd.print("Humidity in %");
  // make output right-aligned
  lcd.setCursor( (lcdColumns - (int(log10(humidity))+1)), 2);
  lcd.print(int(round(humidity)));

  //delay(waitTime);
}
#endif


#ifdef DISPLAY_OLED
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
#endif


#ifdef DISPLAY_LCD
// Parameters:
//   row:        where text will be printed
//   message:    text to scroll
//   delayTime:  time between character shifting
// inspired by https://randomnerdtutorials.com/esp32-esp8266-i2c-lcd-arduino-ide/
void scrollLCDText( int row, String message, int delayTime ){
  // add whitespaces equal to no. LCD-columns at beginning of string
  for (int i=0; i<lcdColumns ; ++i) {
    message = " "+message;
  }
  message = message+" ";
  // emulate the scrolling by printing substrings sequentially
  for (int pos=0 ; pos<message.length(); ++pos){
    lcd.setCursor(0,row);
    lcd.print(message.substring(pos, pos+lcdColumns));
    delay(delayTime);
  }
}
#endif
// -------------------------------------------------------------------
