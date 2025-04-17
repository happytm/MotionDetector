// motionDetector_web.ino with SPIFFS + Preferences
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Preferences.h>

// ==== WiFi CONFIGURATION ====
const char* ssid = "HTM";
const char* password = "";

AsyncWebServer server(80);
Preferences prefs;

// ==== MOTION DETECTOR SETTINGS ====
#define MAX_SAMPLEBUFFERSIZE 256
#define MAX_AVERAGEBUFFERSIZE 64
#define MAX_VARIANCE 65535
#define ABSOLUTE_RSSI_LIMIT -100

int *sampleBuffer = nullptr, *mobileAverageBuffer = nullptr, *varianceBuffer = nullptr;

int sampleBufferSize = MAX_SAMPLEBUFFERSIZE;
int mobileAverageFilterSize = MAX_SAMPLEBUFFERSIZE;
int mobileAverageBufferSize = MAX_AVERAGEBUFFERSIZE;
int mobileAverage = 0;
int mobileAverageTemp = 0;
int sampleBufferIndex = 0;
int sampleBufferValid = 0;
int mobileAverageBufferIndex = 0;
int mobileAverageBufferValid = 0;
int variance = 0;
int variancePrev = 0;
int varianceSample = 0;
int varianceAR = 0;
int varianceIntegral = 0;
int varianceThreshold = 3;
int varianceIntegratorLimitMax = MAX_SAMPLEBUFFERSIZE;
int varianceIntegratorLimit = 3;
int varianceBufferSize = MAX_SAMPLEBUFFERSIZE;
int detectionLevel = 0;
int varianceBufferIndex = 0;
int varianceBufferValid = 0;
int enableCSVout = 0;
int minimumRSSI = ABSOLUTE_RSSI_LIMIT;



int motionDetector_config(int sampleBufSize = 256,int mobileAvgSize = 64,int varThreshold = 3,int varIntegratorLimit = 3,bool enableAR = false)
  {
  if (sampleBufSize > MAX_SAMPLEBUFFERSIZE) { sampleBufSize = MAX_SAMPLEBUFFERSIZE; }
  sampleBufferSize = sampleBufSize;
  varianceBufferSize = sampleBufSize;

  if (mobileAvgSize > MAX_SAMPLEBUFFERSIZE) { mobileAvgSize = MAX_SAMPLEBUFFERSIZE; }
  mobileAverageFilterSize = mobileAvgSize;

  if (varThreshold > MAX_VARIANCE) { varThreshold = MAX_VARIANCE; }
  varianceThreshold = varThreshold;

  if (varIntegratorLimit > varianceIntegratorLimitMax) { varIntegratorLimit = varianceIntegratorLimitMax; }
  varianceIntegratorLimit = varIntegratorLimit;

  
  return 1;
}

// ==== SETUP ====
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  prefs.begin("motiondet", false);
  varianceThreshold = prefs.getInt("threshold", 3);

   // Configure motion detector
  motionDetector_config(128, 32, 5, 5, true); // Example custom settings

  // Allocate memory based on config
  sampleBuffer = (int*)malloc(sizeof(int) * sampleBufferSize);
  mobileAverageBuffer = (int*)malloc(sizeof(int) * mobileAverageBufferSize);
  varianceBuffer = (int*)malloc(sizeof(int) * varianceBufferSize);
  for (int i = 0; i < sampleBufferSize; i++) sampleBuffer[i] = 0;
  for (int i = 0; i < mobileAverageBufferSize; i++) mobileAverageBuffer[i] = 0;
  for (int i = 0; i < varianceBufferSize; i++) varianceBuffer[i] = 0;



  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.on("/data.json", HTTP_GET, [](AsyncWebServerRequest *request){
    int rssi = WiFi.RSSI();
    int val = motionDetector_process(rssi);
    String json = "{";
    json += "\"rssi\":" + String(rssi) + ", ";
    json += "\"variance\":" + String(val);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"threshold\":" + String(varianceThreshold) ;
    json += "}";
    request->send(200, "application/json", json);
  });

 

  server.begin();
}

// ==== LOOP ====
void loop() {
  int rssi = WiFi.RSSI();
  int result = motionDetector_process(rssi);
  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.print(" -> Detection: ");
  Serial.println(result);
  delay(100); // Adjust as needed
}

// ==== CONFIG & INIT ====
void motionDetector_config() {
  sampleBuffer = (int*)malloc(sizeof(int) * sampleBufferSize);
  mobileAverageBuffer = (int*)malloc(sizeof(int) * mobileAverageBufferSize);
  varianceBuffer = (int*)malloc(sizeof(int) * sampleBufferSize);
  for (int i = 0; i < sampleBufferSize; i++) sampleBuffer[i] = 0;
  for (int i = 0; i < mobileAverageBufferSize; i++) mobileAverageBuffer[i] = 0;
  for (int i = 0; i < sampleBufferSize; i++) varianceBuffer[i] = 0;
}

int motionDetector_process(int sample) {
  sampleBuffer[sampleBufferIndex++] = sample;
  if (sampleBufferIndex >= sampleBufferSize) {
    sampleBufferIndex = 0;
    sampleBufferValid = 1;
  }

  if (sampleBufferValid) {
    mobileAverageTemp = 0;
    for (int i = 0; i < mobileAverageFilterSize; i++) {
      int idx = sampleBufferIndex - i;
      if (idx < 0) idx += sampleBufferSize;
      mobileAverageTemp += sampleBuffer[idx];
    }
    mobileAverage = mobileAverageTemp / mobileAverageFilterSize;
    mobileAverageBuffer[mobileAverageBufferIndex++] = mobileAverage;
    if (mobileAverageBufferIndex >= mobileAverageBufferSize) {
      mobileAverageBufferIndex = 0;
      mobileAverageBufferValid = 1;
    }
    varianceSample = (sample - mobileAverage)*(sample - mobileAverage);
    varianceBuffer[varianceBufferIndex++] = varianceSample;
    if (varianceBufferIndex >= sampleBufferSize) {
      varianceBufferIndex = 0;
      varianceBufferValid = 1;
    }

    varianceIntegral = 0;
    for (int i = 0; i < varianceIntegratorLimit; i++) {
      int idx = varianceBufferIndex - i;
      if (idx < 0) idx += sampleBufferSize;
      varianceIntegral += varianceBuffer[idx];
    }

    variance = varianceIntegral;
  }

  return variance;
}
