#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include "time.h"
#include "motionDetector.h" // Thanks to: https://github.com/paoloinverse/motionDetector_esp

int index_html_gz_len = 5850;
const uint8_t index_html_gz[] = {
0x1f,0x8b,0x08,0x08,0xc6,0x0f,0x98,0x62,0x00,0xff,0x69,0x6e,0x64,0x65,0x78,0x2e,0x68,0x74,0x6d,0x6c,0x2e,0x67,0x7a,0x00,0x95,0x56,0xdd,0x6e,0xe3,0x36,0x13,0xbd,0x0f,0xfa,0x10,0x03,0xa3,0x58,0xcb,0x45,0x22,0x3b,0xd9,0xbd,0x58,0x24,0xb6,0x2e,0x36,0xc9,0xa2,0xf9,0x90,0x34,0x41,0xed,0x62,0x6f,0x97,0x16,0xc7,0x16,0xbb,0x92,0xa8,0x8a,0x54,0x54,0x23,0xf0,0xbb,0xef,0x0c,0x49,0xff,0xc8,0x4e,0x82,0x7e,0x06,0x0c,0x51,0xf4,0xe1,0xe1,0xcc,0xe1,0x99,0xa1,0xc7,0x99,0x2d,0xf2,0x04,0x4e,0xc6,0x19,0x0a,0xc9,0xcf,0x02,0xad,0x80,0x34,0x13,0xb5,0x41,0x3b,0xe9,0x35,0x76,0x71,0xf6,0xb9,0x97,0x84,0xe9,0x52,0x14,0x38,0xe9,0x3d,0x2b,0x6c,0x2b,0x5d,0xdb,0x1e,0xa4,0xba,0xb4,0x58,0x12,0xac,0x55,0xd2,0x66,0x13,0x89,0xcf,0x2a,0xc5,0x33,0xf7,0x72,0x0a,0xaa,0x54,0x56,0x89,0xfc,0xcc,0xa4,0x22,0xc7,0xc9,0x79,0x8f,0xc9,0xad,0xb2,0x39,0x26,0x53,0xbd,0xb0,0xad,0xa8,0x11,0x1e,0xb4,0x55,0xba,0x84,0x29,0x96,0x46,0xd7,0xe3,0xa1,0xff,0x95,0x60,0xc3,0x10,0xcc,0xc9,0x78,0xae,0xe5,0x2a,0xa1,0x67,0x76,0x91,0x04,0xb0,0x71,0x60,0x88,0x4a,0x0d,0x14,0xa4,0x74,0x3c,0x35,0xfe,0xd3,0xa8,0x1a,0xe5,0x80,0x56,0x5e,0x50,0xb0,0xd9,0xa7,0x84,0x38,0x25,0x5c,0xeb,0xa2,0x10,0xf4,0xb4,0x1a,0x6e,0xa7,0x4f,0x1f,0x2f,0x60,0x41,0x2b,0xbf,0xa9,0xaf,0x0a,0x3e,0x40,0xd1,0xa1,0xa3,0x64,0xad,0x2a,0x97,0x86,0x08,0x3e,0xf1,0x7e,0x04,0x2c,0x40,0xc9,0x49,0x8f,0x07,0xb7,0x39,0x16,0xa4,0x01,0x50,0x64,0xaa,0xac,0x1a,0x1b,0x74,0xc0,0x52,0xcc,0x73,0xec,0x81,0x5d,0x55,0xf4,0x66,0xf1,0x5f,0x52,0xa4,0xca,0x45,0x8a,0x99,0xce,0x25,0xd6,0x93,0xde,0xad,0x03,0xc0,0x04,0xce,0x81,0xb6,0xb8,0x51,0x26,0xbc,0x8e,0x7a,0x43,0x0a,0x72,0x9f,0x8a,0x34,0x2a,0x15,0x69,0x59,0x3f,0x8b,0xfc,0x1d,0xc2,0x29,0xc1,0xe0,0x2e,0xe0,0x7e,0x3b,0x1f,0x45,0x1f,0x47,0x9c,0xdb,0xf9,0x68,0x34,0x38,0xa2,0xb4,0x59,0x8d,0x86,0x17,0xbe,0xc3,0x77,0x83,0x16,0x53,0x27,0xc3,0x6c,0x8b,0x3e,0xe4,0xa9,0x8d,0x51,0xef,0x50,0x3c,0xd0,0x31,0x17,0x4d,0x01,0x7f,0x4e,0xa7,0x77,0x91,0xc4,0x85,0x68,0x72,0x4b,0x29,0x7e,0x0e,0x11,0xcd,0x1b,0x6b,0x89,0xde,0x2f,0x37,0xcd,0xbc,0x50,0xb6,0x47,0x67,0x63,0x37,0x47,0x7f,0x24,0xbf,0x5f,0xc0,0x47,0x30,0xaf,0x13,0xfe,0xd2,0x68,0x3f,0x1c,0xda,0xe6,0xe6,0x3d,0x85,0xf8,0xe7,0xc3,0x14,0x9e,0x84,0x31,0xad,0xae,0xb7,0x4a,0x54,0xdb,0xf7,0xce,0xda,0x2d,0xec,0xcd,0xc0,0x6f,0x59,0x7a,0x6f,0xa0,0x6b,0x5d,0x2e,0xd4,0x72,0x17,0xef,0x5e,0xb8,0x43,0xf6,0x0c,0x0f,0x4c,0x5a,0xab,0xca,0xb2,0x71,0x36,0x2e,0x8a,0x75,0xe9,0xb9,0x48,0x22,0x61,0x56,0x65,0x0a,0x11,0x0e,0x60,0x92,0xc0,0x0b,0x81,0x00,0x30,0xae,0x6a,0x7c,0xa6,0x8a,0xba,0xf1,0x42,0x46,0x83,0xab,0x13,0xf7,0x43,0x4e,0x8a,0xd1,0x01,0x55,0xb4,0x9c,0x0d,0x24,0x5a,0x41,0x14,0x0b,0xb4,0x69,0x16,0xf5,0x87,0x95,0x36,0xb6,0x7f,0x1a,0x28,0x00,0xa8,0x52,0x33,0x2d,0x2f,0xa1,0xff,0xf4,0x38,0x9d,0xf5,0x4f,0xc3,0x2c,0x17,0xd2,0x25,0x94,0xd8,0xc2,0x57,0x8a,0xe5,0x46,0x58,0x11,0x6d,0x82,0x1a,0x38,0xc8,0x9a,0xf6,0xe2,0x27,0xd5,0xb4,0xd1,0x39,0xc6,0xb9,0x5e,0xee,0x10,0xfe,0xa7,0x10,0x85,0x3f,0x61,0x1f,0xc3,0x26,0xa8,0xf8,0x6f,0xa3,0xcb,0x6d,0xb8,0x54,0xf0,0xb5,0x8d,0x3c,0x34,0x2e,0xd0,0x18,0xb1,0x44,0xc7,0xb1,0xbe,0x22,0x79,0x36,0xb2,0x9c,0x8c,0xab,0xe4,0xba,0xa9,0x6b,0xca,0x77,0x63,0x87,0x7b,0xca,0x3e,0xbf,0x84,0xb1,0xa9,0xc8,0xe8,0x5c,0x7e,0xbe,0x4a,0xcf,0x72,0x9e,0x67,0xe3,0x38,0xb7,0x50,0x43,0x51,0xf3,0x5a,0xb0,0x63,0xe2,0x38,0x26,0x42,0x42,0x27,0xe3,0x61,0x45,0xa7,0x50,0x25,0xf7,0xc2,0xd8,0x4d,0x71,0x4b,0xe7,0x6f,0x94,0x20,0xec,0x3e,0xa9,0x55,0x05,0xf6,0x92,0x6f,0x14,0x3f,0x31,0xb8,0x9e,0xd0,0xc1,0xd3,0xe8,0x0d,0xda,0xfd,0x28,0xa1,0x15,0x66,0x9f,0x34,0x27,0xc0,0x59,0x37,0xdc,0xff,0x63,0x87,0x9d,0x59,0x9e,0x45,0x0d,0x4b,0x61,0xb1,0x15,0x2b,0x52,0xf9,0x7b,0x6b,0x2e,0x87,0xc3,0x5f,0x5f,0x5a,0x55,0x4a,0xdd,0xd2,0xa1,0xa4,0x94,0x36,0xad,0xce,0xe8,0xcc,0xd9,0xdc,0xeb,0x61,0x6b,0xbe,0x5f,0xb9,0x45,0x2d,0xce,0x8d,0x4e,0x7f,0xa0,0xa5,0x43,0x08,0x70,0x21,0xe5,0x2d,0xdb,0xe9,0x5e,0x19,0xea,0xd3,0x58,0x47,0xfd,0x5c,0x0b,0x49,0x5e,0xd1,0xe5,0x3d,0x0d,0xf8,0xb8,0x16,0x4d,0xe9,0x3b,0x80,0x9f,0x8a,0x9c,0xfd,0x06,0xc1,0x4c,0xdc,0xc2,0xbf,0xe1,0x7c,0xea,0x68,0xf9,0x74,0x79,0xf2,0x64,0xbd,0xb7,0xea,0x00,0x11,0xd6,0xed,0x7b,0xa8,0x3f,0xab,0x57,0x2c,0x02,0xf5,0x29,0x5d,0x61,0x09,0x02,0xb6,0x78,0xc6,0x95,0x5b,0x39,0xfa,0x81,0x7f,0x9b,0x07,0xa5,0xcf,0x7e,0xdd,0xd1,0x07,0x59,0x0e,0x71,0x54,0x54,0x8e,0x99,0x3e,0x13,0x4a,0xe3,0x91,0xc6,0xc7,0x88,0x34,0xd7,0x54,0x3a,0x1e,0x71,0xcd,0xe3,0x63,0x48,0x30,0xaa,0x83,0x3c,0xf8,0xf1,0x55,0x27,0x59,0xcf,0xdd,0x95,0xa8,0x93,0x2a,0x35,0x85,0x90,0x8f,0xcb,0x15,0x25,0xe7,0xd4,0x65,0x70,0x7b,0xff,0x37,0x0a,0x17,0xb2,0xa3,0x60,0x14,0xb5,0xc8,0x19,0xf9,0x56,0x37,0x36,0xea,0x88,0x7e,0x0a,0x17,0x23,0xea,0xff,0x87,0xfb,0x84,0x04,0xba,0x3b,0x71,0x01,0x4b,0x2a,0x7d,0x4a,0xf1,0x7f,0xd3,0xc7,0x3f,0xe2,0x8a,0x6f,0x79,0x0f,0x89,0x79,0xfe,0x95,0x16,0xb0,0x37,0xed,0x2d,0xb1,0x00,0x37,0x07,0x63,0xbe,0x76,0x46,0xc4,0x2c,0x75,0xda,0x14,0xcc,0xb0,0x44,0xcb,0xcd,0x82,0x86,0x5f,0x56,0x77,0x32,0xea,0xd6,0xee,0x20,0x56,0x94,0x58,0xfd,0xfb,0xec,0xe1,0x9e,0x76,0x67,0x86,0xf5,0x2b,0xa4,0xc9,0x86,0xd4,0xcf,0xf3,0x87,0xad,0xdd,0xd8,0x74,0x8a,0x14,0x95,0x34,0x61,0xed,0x95,0x9b,0x96,0xc1,0x21,0xd4,0xcd,0x30,0x22,0x09,0x60,0x38,0xa4,0x8b,0x0c,0x81,0x2e,0xc5,0x0c,0xe9,0x6f,0x81,0x32,0x3c,0x80,0x1f,0xb8,0x3a,0x85,0x36,0x53,0x69,0xc6,0x22,0xfa,0x39,0x22,0x41,0xb6,0x24,0x8f,0xb1,0xd2,0x69,0xb6,0xdd,0x4f,0xc6,0x04,0xfa,0x6b,0x76,0x1d,0x36,0x8c,0x76,0x7b,0x07,0x19,0x1c,0xe8,0xad,0x9c,0x5d,0x6b,0x39,0xc8,0x75,0xbb,0x6a,0xbd,0xc9,0xea,0x95,0xa4,0xe1,0xc3,0x07,0x7f,0x34,0x41,0xd7,0xb7,0x65,0x3d,0x6e,0x34,0xaf,0x69,0x7b,0x58,0xa4,0x5f,0xdc,0x3d,0xb5,0xad,0xd0,0xb7,0xd8,0xfb,0x56,0x2f,0x97,0x39,0xf6,0x07,0xaf,0xf4,0x8e,0x34,0x57,0xe9,0x0f,0xd7,0x3c,0x66,0x0e,0x74,0xe4,0x39,0x3f,0xdd,0xb5,0xdc,0xae,0xc0,0xe8,0xaa,0x97,0x91,0xb3,0x9d,0xb1,0x35,0x75,0x04,0xb5,0x58,0x45,0x2f,0x7d,0xe1,0x16,0xf7,0x2f,0x37,0x1b,0xaf,0x07,0x8e,0x76,0xef,0x96,0xe0,0x1b,0xd5,0xff,0x0d,0xa4,0x3f,0x67,0xee,0xcf,0x2a,0xfc,0xf2,0x13,0xae,0x9a,0x90,0x44,0xb6,0x0a,0x00,0x00
};

struct tm timeinfo;
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0" //(New York) https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

#define FIRSTTIME  false   // Define true if setting up Gateway for first time.

const char* apSSID = "ESP";
const char* apPassword = "";
const int apChannel = 7;
const int hidden = 0;                 // If hidden is 1 probe request event handling does not work ?

 //==================User configuration not required below this line ================================================

//int enableCSVgraphOutput = 1;    // 0 disable, 1 enable // if enabled, you may use Tools-> Serial Plotter to plot the variance output for each transmitter. 
int motionLevel = -1; // initial value = -1, any values < 0 are errors, see motionDetector.h , ERROR LEVELS sections for details on how to intepret any errors.
unsigned long time_now = 0;

String ssid,password;
uint8_t receivedCommand[6],showConfig[20];
const char* ntpServer = "pool.ntp.org";
unsigned long epoch; 

AsyncWebServer webserver(80);
AsyncWebSocket ws("/ws");

void notifyClients(String level) {
  ws.textAll(level);
  Serial.print("Websocket message sent: ");Serial.println(level);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
    {
      
      // Process incoming message.
      data[len] = 0;
      String message = "";
      message = (char*)data;
    
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) 
{
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

unsigned long getTime() {time_t now;if (!getLocalTime(&timeinfo)) {Serial.println("Failed to obtain time");return(0);}Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");time(&now);return now;}

void setup(){
  Serial.begin(115200);
  delay(500);
  EEPROM.begin(512);

#if FIRSTTIME  
  // Setup device numbers and wifi Channel for remote devices in EEPROM permanantly.
  
  EEPROM.writeByte(0, 0);          // Enable/disable motion sensor.
  EEPROM.writeByte(1, 100);        // Scan interval for motion sensor.
  EEPROM.writeByte(2, 100);        // Level threshold where motion is considered valid.
  EEPROM.writeByte(3, 80);         // Minimum RSSI level to be considered for reliable motion detection.
  EEPROM.commit();
#endif  
  
  EEPROM.readBytes(0, showConfig,20);for(int i=0;i<20;i++){Serial.printf("%d ", showConfig[i]);}
  Serial.println();
  
  startWiFi();
  startAsyncwebserver();
  
  motionDetector_init();  // initializes the storage arrays in internal RAM
  motionDetector_config(64, 16, 3, 3, false); 
  Serial.setTimeout(1000);
    
  configTime(0, 0, ntpServer); setenv("TZ", MY_TZ, 1); tzset(); // Set environment variable with your time zone
          
  WiFi.onEvent(probeRequest, SYSTEM_EVENT_AP_PROBEREQRECVED); Serial.print("Waiting for probe requests ... ");

} // End of setup

void loop()
{
   
 if(millis() >= time_now + (EEPROM.readByte(1) * 10))
  {      // Implementation of non blocking delay function.
   Serial.print("Motion sensor scan interval: ");Serial.println(EEPROM.readByte(1) * 10);
   time_now += (EEPROM.readByte(1) * 10);
   
   Serial.print("Motion sensor Status: "); Serial.println(EEPROM.readByte(0));
   if (EEPROM.readByte(0) == 1) // Only process following code if motion sensor Enabled.
    {
     Serial.print("Motion sensor minimum RSSI value set to: ");Serial.println(EEPROM.readByte(3) * -1); // motionDetector_set_minimum_RSSI(EEPROM.readByte(5) * -1); // Minimum RSSI value to be considered reliable. Default value is 80 * -1 = -80. 
     notifyClients(String(motionLevel));
     
     unsigned long lastDetected;      // Epoch time at which last motion level detected above trigger threshold.
     
     //Serial.print("Motion sensor Threshold set to: ");Serial.println(EEPROM.readByte(4));
     if (motionLevel > EEPROM.readByte(2))
      {
       lastDetected = getTime();
      // lastLevel = motionLevel;
      }
       notifyClients(String(lastDetected));
       ws.cleanupClients();
   
       motionLevel = motionDetector_esp();  // if the connection fails, the radar will automatically try to switch to different operating modes by using ESP32 specific calls. 
       Serial.print("Motion Level: ");
       Serial.println(motionLevel);
    } 
  }
       if (WiFi.waitForConnectResult() != WL_CONNECTED) {ssid = EEPROM.readString(270); password = EEPROM.readString(301);Serial.println("Wifi connection failed");Serial.print("Connect to Access Point ");Serial.print(apSSID);Serial.println(" and point your browser to 192.168.4.1 to set SSID and password" );WiFi.disconnect(false);delay(1000);WiFi.begin(ssid.c_str(), password.c_str());}
       
}  // End of loop


void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{ 
  Serial.println();
  Serial.print("Probe Received :  ");for (int i = 0; i < 6; i++) {Serial.printf("%02X", info.ap_probereqrecved.mac[i]);if (i < 5)Serial.print(":");}Serial.println();
  Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.print(" or 192.168.4.1 with connection to ESP AP");Serial.println(" to monitor and control whole network");
} // End of Proberequest function.


void startWiFi()
{
  WiFi.mode(WIFI_AP_STA);
  
  WiFi.softAP(apSSID, apPassword, apChannel, hidden);
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
  Serial.print("AP started with name: ");Serial.println(apSSID);
  
  ssid = EEPROM.readString(21); password = EEPROM.readString(51);
  WiFi.begin(ssid.c_str(), password.c_str());
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wifi connection failed");
    Serial.print("Connect to Access Point '");Serial.print(apSSID);Serial.println("' and point your browser to 192.168.4.1 to set SSID and password");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid.c_str(), password.c_str());
 }
   
    Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.print(" or 192.168.4.1");Serial.println(" to monitor and control whole network");
}


void startAsyncwebserver()
{
  
  ws.onEvent(onEvent);
  webserver.addHandler(&ws);
  
  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest * request) 
    {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, sizeof(index_html_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
    });
      
  webserver.on("/post", HTTP_POST, [](AsyncWebServerRequest * request){
  
  int params = request->params();
  
  for(int i=0;i<params;i++){
  AsyncWebParameter* p = request->getParam(i);
    
    String input0 =request->getParam(0)->value();receivedCommand[0] =(atoi(input0.c_str()));
    String input1 =request->getParam(1)->value();receivedCommand[1] =(atoi(input1.c_str()));  
    String input2 =request->getParam(2)->value();receivedCommand[2] =(atoi(input2.c_str()));
    String input3 =request->getParam(3)->value();receivedCommand[3] =(atoi(input3.c_str())); 
    ssid = request->getParam(4)->value().c_str();                  
    password =request->getParam(5)->value().c_str();
  /*       
  if(p->isPost()){
    Serial.printf("Command[%s]: %s\n", p->name().c_str(), p->value()); // For debug purpose.
    }
  */
} 
  request -> send(200, "text/plain", "Command received by server successfully, please click browser's back button to get back to main page.");
  Serial.print("Command received from Browser: ");Serial.print(receivedCommand[0]);Serial.print(receivedCommand[1]);Serial.print(receivedCommand[2]);Serial.print(receivedCommand[3]);Serial.print(receivedCommand[4]);Serial.println(receivedCommand[5]);

  if (ssid.length() > 0 || password.length() > 0) 
  {  
    EEPROM.writeString(21,ssid);EEPROM.writeString(51, password);
    EEPROM.commit();Serial.println();Serial.print("Wifi Configuration saved to EEPROM: SSID="); Serial.print(ssid);Serial.print(" & Password="); Serial.println(password);Serial.println("Restarting Gateway now...");delay(1000);
    ESP.restart();
  }
    for (int i = 0; i < 4; i++) 
      {
       uint8_t motionSettings[4]; // Enable/diable motion sensor, Scan interval, Level threshold & minimum RSSI.
       motionSettings[i] = receivedCommand[i];
       EEPROM.writeBytes(0, motionSettings,4);
      }
      EEPROM.commit();
      EEPROM.readBytes(0, showConfig,10);for(int i=0;i<10;i++){Serial.printf("%d ", showConfig[i]);}Serial.println();
}); 
  
      
  //Following line must be added before server.begin() to allow local lan request see : https://github.com/me-no-dev/ESPAsyncWebServer/issues/726
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    webserver.begin();
}
