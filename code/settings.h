#ifndef SETTINGS_H
#define SETTINGS_H

// -------------------------------------------------------------------
// Some switches defining general behaviour of the program
// -------------------------------------------------------------------
#define WIFI_ENABLED true      // set to true if WiFi is desired, 
                                // otherwise corresponding code is not compiled
#define WIFI_WEBSERVER false   // website in your WiFi for data and data logger
#define WIFI_MQTT true       // activate MQTT integration in your WiFi
#define MQTT_SERVER_IP "192.168.1.100"
#define MQTT_TOPIC_PREFIX "esp-co2/co2/"  // MQTT Topic prefix, this prefix will get
                                            // sub-topics vcc, hostname, co2, temp, hum
                                            // should be 20 characters maximum (because of char mqttTopic[28]; )
                                            // Please make sure to add / as last character
#define DEBUG true              // activate debugging
                                // true:  print info + data to serial monitor
                                // false: serial monitor is not used
//#define DISPLAY_OLED            // OLED display
//#define DISPLAY_LCD           // LCD display
#define SEND_VCC false

#define ESP_DEVICE_NAME "CO2Monitor"        // ESP Device name displayed via MQTT

// -------------------------------------------------------------------
// Hardware configurations and some global constants
// -------------------------------------------------------------------
#define CO2_THRESHOLD1 600
#define CO2_THRESHOLD2 1000
#define CO2_THRESHOLD3 1500

#define WARNING_DIODE_PIN D8      // NodeMCU pin for red LED

#define MEASURE_INTERVAL 10       // seconds, minimum: 2 

#define SCREEN_WIDTH 128          // OLED display width in pixels
#define SCREEN_HEIGHT 32          // OLED display height in pixels

#define TEMP_OFFSET 0             // temperature offset of the SCD30
                                  // 0 is default value
                                  // 5 is used in SCD30-library example
                                  // 5 also works for most of my devices
                                  // Just changes the transmitted value by -Offset?

#define ALTITUDE_OFFSET 300       // altitude of place of operation in meters
                                  // Stuttgart: approx 300; Uni Stuttgart: approx 500; Lohne: 67

#define OLED_RESET LED_BUILTIN    // OLED reset pin, 4 is default
                                  // -1 if sharing Arduino reset pin
                                  // using NodeMCU, it is LED_BUILTIN




#endif
