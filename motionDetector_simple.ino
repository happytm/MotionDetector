#include <WiFi.h>
#include "motionDetector.h"    // Thanks to https://github.com/paoloinverse/motionDetector_esp

const char* ssid = "ESP";      // Required for OTA update.
const char* password = "";     // Required for OTA update.

int enableCSVgraphOutput = 1;  // 0 disable, 1 enable // if enabled, you may use Tools-> Serial Plotter to plot the variance output for each transmitter. 
int motionLevel = -1;
//==================User configuration generally not required below this line ============================

void setup(){
  Serial.begin(115200);
  Serial.println();
  
  motionDetector_init();  // initializes the storage arrays in internal RAM
  motionDetector_config(64, 16, 3, 3, false); 
  Serial.setTimeout(1000);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wifi connection failed");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
   }
}

//===================== End of Setup ====================================================

void loop(){ 
  
 motionDetector_set_minimum_RSSI(-80);     // Minimum RSSI value to be considered reliable. Default value is 80 * -1 = -80.
 motionLevel = 0;  // Reset motionLevel to 0 to resume motion tracking.
 motionLevel = motionDetector_esp();  // if the connection fails, the radar will automatically try to switch to different operating modes by using ESP32 specific calls. 
 //Serial.print("Motion detected & motion level is: ");Serial.println(motionLevel);

 if (motionLevel > 15)  // Adjust the sensitivity of motion sensor.Higher the number means less sensetive motion sensor is.
 {     
  Serial.print("Motion detected & motion level is: ");Serial.println(motionLevel);
  delay(500);
 }
} // End of loop.
