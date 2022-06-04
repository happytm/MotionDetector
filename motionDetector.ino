#define FIRSTTIME  false    // Define true if setting up this device for first time.

#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include <FS.h>
#include "LITTLEFS.h"
#include <SPIFFSEditor.h>
#include "time.h"
#include <TinyMqtt.h>       // Thanks to https://github.com/hsaturn/TinyMqtt
#include "motionDetector.h" // Thanks to https://github.com/paoloinverse/motionDetector_esp
#include <ESP32Ping.h>      // Thanks to https://github.com/marian-craciunescu/ESP32Ping

std::string sentTopic = "data";
std::string receivedTopic = "command";

struct tm timeinfo;
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0" //(New York) https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

char* room = "Livingroom";  // Needed for person locator.Each location must run probeReceiver sketch to implement person locator.
int rssiThreshold = -50;    // Adjust according to signal strength by trial & error.

IPAddress deviceIP(192, 168, 0, 2); // Fixed IP address assigned to family member's devices to be checked for their presence at home.
//IPAddress deviceIP = WiFi.localIP();
int device1IP = 2, device2IP = 3, device3IP = 4, device4IP = 5;
uint8_t device1ID[3] = {0xD0, 0xC0, 0x8A};   // First and last 2 bytes of Mac ID of Cell phone #1.
uint8_t device2ID[3] = {0x36, 0x33, 0x33};   // First and last 2 bytes of Mac ID of Cell phone #2.
uint8_t device3ID[3] = {0x36, 0x33, 0x33};   // First and last 2 bytes of Mac ID of Cell phone #3.
uint8_t device4ID[3] = {0x36, 0x33, 0x33};   // First and last 2 bytes of Mac ID of Cell phone #4.

const char* apSSID = "ESP";
const char* apPassword = "";
const int apChannel = 7;
const int hidden = 0;                 // If hidden is 1 probe request event handling does not work ?

const char* http_username = "admin";  // Web file editor interface Login.
const char* http_password = "admin";  // Web file editor interface password.

String dataFile = "/data.json";       // File to store sensor data.


 //==================User configuration not required below this line ================================================

int RSSIlevel, motionLevel = -1; // initial value = -1, any values < 0 are errors, see motionDetector.h , ERROR LEVELS sections for details on how to intepret any errors.
String ssid,password, graphData;
uint8_t receivedCommand[6],showConfig[20];
const char* ntpServer = "pool.ntp.org";
unsigned long time_now, epoch, lastDetected;      // Epoch time at which last motion level detected above trigger threshold.

#define MYFS LITTLEFS
#define FORMAT_LITTLEFS_IF_FAILED true

MqttBroker broker(1883);
MqttClient myClient(&broker);

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
  LITTLEFS.begin();
  EEPROM.begin(512);

#if FIRSTTIME       
  firstTimeSetup();    // Setup device numbers and wifi Channel for remote devices in EEPROM permanantly.
#endif
  
  EEPROM.readBytes(0, showConfig,20);for(int i=0;i<20;i++){Serial.printf("%d ", showConfig[i]);}
  Serial.println();
  
  startWiFi();
  startAsyncwebserver();
  
  motionDetector_init();  // initializes the storage arrays in internal RAM
  motionDetector_config(64, 16, 3, 3, false); 
  Serial.setTimeout(1000);
    
  configTime(0, 0, ntpServer); setenv("TZ", MY_TZ, 1); tzset(); // Set environment variable with your time zone
  
  broker.begin();
  myClient.setCallback(receivedMessage);
  myClient.subscribe(receivedTopic);
  myClient.subscribe(sentTopic);        
  
  WiFi.onEvent(probeRequest, SYSTEM_EVENT_AP_PROBEREQRECVED); Serial.print("Waiting for probe requests ... ");

} // End of setup

void loop()
{
   
 if(millis() >= time_now + (EEPROM.readByte(1) * 10))
  {      // Implementation of non blocking delay function.
   Serial.print("Motion sensor scan interval: ");Serial.println(EEPROM.readByte(1) * 10);
   time_now += (EEPROM.readByte(1) * 10);
         
      if (EEPROM.readByte(0) == 1) // Only process following code if motion sensor Enabled.
       {
         Serial.print("Motion sensor Status: "); Serial.println("Enabled");
         Serial.print("Motion sensor minimum RSSI value set to: ");Serial.println(EEPROM.readByte(3) * -1); //motionDetector_set_minimum_RSSI = (EEPROM.readByte(5) * -1); // Minimum RSSI value to be considered reliable. Default value is 80 * -1 = -80. 
    
         notifyClients(String(motionLevel));
              
     //Serial.print("Motion sensor Threshold set to: ");Serial.println(EEPROM.readByte(4));
     if (motionLevel > EEPROM.readByte(2)) // If motion is detected.
      {
       lastDetected = getTime();
       // If motion detected, check any family member is at home.
       
       Serial.println("Checking if anybody at Home.... ");
        
       int pingTime, fatherPing, motherPing, sonPing, daughterPing;
         
         deviceIP[3] = device1IP;
         Serial.println("Pinging IP address 2... ");
         if(Ping.ping(deviceIP)) {Serial.println("Father is at Home");notifyClients(String(2020));} else { Serial.println("Father is Away");notifyClients(String(2000));}
         fatherPing = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(fatherPing);
         deviceIP[3] = device2IP;
         Serial.println("Pinging IP address 3... ");
         if(Ping.ping(deviceIP)) {Serial.println("Mother is at Home");notifyClients(String(3030));} else { Serial.println("Mother is Away");notifyClients(String(3000));}
         motherPing = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(motherPing);
         deviceIP[3] = device3IP;
         Serial.println("Pinging IP address 4... ");
         if(Ping.ping(deviceIP)) {Serial.println("Son is at Home");notifyClients(String(4040));} else { Serial.println("Son is Away");notifyClients(String(4000));}
         sonPing = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(sonPing);
         deviceIP[3] = device4IP;
         Serial.println("Pinging IP address 5... ");
         if(Ping.ping(deviceIP)) {Serial.println("Daughter is at Home");notifyClients(String(5050));} else { Serial.println("Daughter is Away");notifyClients(String(5000));}
         daughterPing = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(daughterPing);

         RSSIlevel = WiFi.RSSI();
         
         graphData = ",";graphData += lastDetected;graphData += ",";graphData += motionLevel;graphData += ",";graphData += RSSIlevel;graphData += ",";graphData += fatherPing;graphData += ",";graphData += motherPing;graphData += ",";graphData += sonPing;graphData += ",";graphData += daughterPing;graphData += "]";
     
         File f = LITTLEFS.open(dataFile, "r+"); Serial.print("File size: "); Serial.println(f.size());  // See https://github.com/lorol/LITTLEFS/issues/33
         f.seek((f.size()-1), SeekSet);Serial.print("Position: "); Serial.println(f.position());
         f.print(graphData);Serial.println();Serial.print("Appended to file: "); Serial.println(graphData);Serial.print("File size: "); Serial.println(f.size());
         f.close(); 
      }
       notifyClients(String(lastDetected));
       ws.cleanupClients();
 
    } else { Serial.print("Motion sensor Status: "); Serial.println("Disabled");}
       motionLevel = 0;  // Reset motionLevel to 0 to resume motion tracking.
       motionLevel = motionDetector_esp();  // if the connection fails, the radar will automatically try to switch to different operating modes by using ESP32 specific calls. 
       Serial.print("Motion Level: ");
       Serial.println(motionLevel);
  }  
       if (WiFi.waitForConnectResult() != WL_CONNECTED) {ssid = EEPROM.readString(270); password = EEPROM.readString(301);Serial.println("Wifi connection failed");Serial.print("Connect to Access Point ");Serial.print(apSSID);Serial.println(" and point your browser to 192.168.4.1 to set SSID and password" );WiFi.disconnect(false);delay(1000);WiFi.begin(ssid.c_str(), password.c_str());}
}  // End of loop


void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{ 
  Serial.println();
  Serial.print("Probe Received :  ");for (int i = 0; i < 6; i++) {Serial.printf("%02X", info.ap_probereqrecved.mac[i]);if (i < 5)Serial.print(":");}Serial.println();
  Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.print(" or 192.168.4.1 with connection to ESP AP");Serial.println(" to monitor and control whole network");

  if (info.ap_probereqrecved.mac[0] == device1ID[0] && info.ap_probereqrecved.mac[4] == device1ID[1] && info.ap_probereqrecved.mac[5] == device1ID[2]) 
  { // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
    
    Serial.println("################ Person 1 arrived ###################### ");
    
    myClient.publish("Sensordata/Person1/", "Home");
    Serial.print("Signal Strength of device: ");
    Serial.println(info.ap_probereqrecved.rssi);
    myClient.publish("Sensordata/Signal/", (String)info.ap_probereqrecved.rssi);
    if (info.ap_probereqrecved.rssi > rssiThreshold) // Adjust according to signal strength by trial & error.
     { // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
       myClient.publish("Sensordata/Person1/in/", room);
     }
              
    }
 } // End of Proberequest function.

void firstTimeSetup() {
 
  EEPROM.writeByte(0, 0);          // Enable/disable motion sensor.
  EEPROM.writeByte(1, 100);        // Scan interval for motion sensor.
  EEPROM.writeByte(2, 100);        // Level threshold where motion is considered valid.
  EEPROM.writeByte(3, 80);         // Minimum RSSI level to be considered for reliable motion detection.
  EEPROM.commit();
}

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


  webserver.serveStatic("/", MYFS, "/").setDefaultFile("index.html");
   
  webserver.addHandler(new SPIFFSEditor(MYFS, http_username,http_password));

  webserver.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
      Serial.printf("%s", (const char*)data);
      if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  
   webserver.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
      Serial.printf("%s", (const char*)data);
      if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });

    //Following line must be added before server.begin() to allow local lan request see : https://github.com/me-no-dev/ESPAsyncWebServer/issues/726
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    webserver.begin();
}

void receivedMessage(const MqttClient* source, const Topic& topic, const char* payload, size_t length)
{
  Serial.print("Received message on topic '"); Serial.print(receivedTopic.c_str());Serial.print("' with payload = ");Serial.println(payload);  
  if (receivedTopic == "command") // Each part of Mqtt commands must be 3 digits (for example: 006 for 6). 
  {
    receivedCommand[0] = atoi(&payload[0]);receivedCommand[1] = atoi(&payload[4]);receivedCommand[2] = atoi(&payload[8]);receivedCommand[3] = atoi(&payload[12]);receivedCommand[4] = atoi(&payload[16]);receivedCommand[5] = atoi(&payload[20]);
    Serial.print("Command received via MQTT: ");Serial.print(receivedCommand[0]);Serial.print(receivedCommand[1]);Serial.println(receivedCommand[2]);
    }
    for (int i = 0; i < 4; i++) 
      {
       uint8_t motionSettings[4]; // Enable/diable motion sensor, Scan interval, Level threshold & minimum RSSI.
       motionSettings[i] = receivedCommand[i];
       EEPROM.writeBytes(0, motionSettings,4);
      }
      EEPROM.commit();
      EEPROM.readBytes(0, showConfig,10);for(int i=0;i<10;i++){Serial.printf("%d ", showConfig[i]);}Serial.println();
}
