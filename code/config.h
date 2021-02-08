#ifndef CONFIG_H
#define CONFIG_H

// -------------------------------------------------------------------
// Some switches defining general behaviour of the program
// -------------------------------------------------------------------
#define WIFI_ENABLED false      // set to true if WiFi is desired, 
                                // otherwise corresponding code is not compiled
#if WIFI_ENABLED
  #define WIFI_WEBSERVER true   // website in your WiFi for data and data logger
  #define WIFI_MQTT false       // activate MQTT integration in your WiFi
  #define MQTT_SERVER_IP "192.168.1.100"

  #define MQTT_USERNAME "mqttuser"      // If your broker needs credentials
  #define MQTT_PASSWORD "mqttpassword"  // add here ;-)
  
  #define MQTT_TOPIC_PREFIX "esp-co2/co2/"  // MQTT Topic prefix, this prefix will get
                                            // sub-topics vcc, hostname, co2, temp, hum
#endif
#define DEBUG true              // activate debugging
                                // true:  print info + data to serial monitor
                                // false: serial monitor is not used
#define DISPLAY_OLED            // OLED display
//#define DISPLAY_LCD           // LCD display
#define SEND_VCC false

#define ESP_DEVICE_NAME         // ESP Device name displayed via MQTT



#endif
