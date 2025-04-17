#include <WiFi.h>

const char* ssid = "HTM";
const char* password = "";
int sensitivity = 25;  // Adujust sensitivity of motion sensor.

// ==== MOTION DETECTOR SETTINGS ====
#define MAX_sampleSize 256
#define MAX_AVERAGEBUFFERSIZE 64
#define MAX_VARIANCE 65535
#define ABSOLUTE_RSSI_LIMIT -100

int *sampleBuffer = nullptr, *averageBuffer = nullptr, *varianceBuffer = nullptr;

int sampleSize = MAX_sampleSize;
int averageFilterSize = MAX_sampleSize;
int averageBufferSize = MAX_AVERAGEBUFFERSIZE;
int average = 0;
int averageTemp = 0;
int sampleBufferIndex = 0;
int sampleBufferValid = 0;
int averageBufferIndex = 0;
int averageBufferValid = 0;
int variance = 0;
int variancePrev = 0;
int varianceSample = 0;
int varianceAR = 0;
int varianceIntegral = 0;
int varianceThreshold = 3;
int varianceIntegratorLimitMax = MAX_sampleSize;
int varianceIntegratorLimit = 3;
int varianceBufferSize = MAX_sampleSize;
int detectionLevel = 0;
int varianceBufferIndex = 0;
int varianceBufferValid = 0;
int enableCSVout = 0;
int minimumRSSI = ABSOLUTE_RSSI_LIMIT;

int motionSensorConfig(int sampleBufSize = 256,int mobileAvgSize = 64,int varThreshold = 3,int varIntegratorLimit = 3,bool enableAR = false)
  {
  if (sampleBufSize > MAX_sampleSize) { sampleBufSize = MAX_sampleSize; }
  sampleSize = sampleBufSize;
  varianceBufferSize = sampleBufSize;

  if (mobileAvgSize > MAX_sampleSize) { mobileAvgSize = MAX_sampleSize; }
  averageFilterSize = mobileAvgSize;

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
  Serial.println(WiFi.localIP());
 
  // Configure motion sensor
  motionSensorConfig(128, 32, 5, 5, true); // Example custom settings

  // Allocate memory based on config
  sampleBuffer = (int*)malloc(sizeof(int) * sampleSize);
  averageBuffer = (int*)malloc(sizeof(int) * averageBufferSize);
  varianceBuffer = (int*)malloc(sizeof(int) * varianceBufferSize);
  for (int i = 0; i < sampleSize; i++) sampleBuffer[i] = 0;
  for (int i = 0; i < averageBufferSize; i++) averageBuffer[i] = 0;
  for (int i = 0; i < varianceBufferSize; i++) varianceBuffer[i] = 0;
}

// ==== LOOP ====
void loop() {
  int rssi = WiFi.RSSI();
  int motion = motionSensor(rssi); 
  //Serial.print("RSSI: "); Serial.print(rssi); Serial.print(" -> Detection: "); Serial.println(motion);
  
  if (motion > sensitivity) {Serial.print(" Motion level detected: "); Serial.println(motion);}
  delay(100); // Adjust as needed
}

int motionSensor(int sample) {
  sampleBuffer[sampleBufferIndex++] = sample;
  if (sampleBufferIndex >= sampleSize) {
    sampleBufferIndex = 0;
    sampleBufferValid = 1;
}

  if (sampleBufferValid) {
    averageTemp = 0;
    for (int i = 0; i < averageFilterSize; i++) {
      int idx = sampleBufferIndex - i;
      if (idx < 0) idx += sampleSize;
      averageTemp += sampleBuffer[idx];
    }
    average = averageTemp / averageFilterSize;
    averageBuffer[averageBufferIndex++] = average;
    if (averageBufferIndex >= averageBufferSize) {
      averageBufferIndex = 0;
      averageBufferValid = 1;
    }
    varianceSample = (sample - average)*(sample - average);
    varianceBuffer[varianceBufferIndex++] = varianceSample;
    if (varianceBufferIndex >= sampleSize) {
      varianceBufferIndex = 0;
      varianceBufferValid = 1;
    }

    varianceIntegral = 0;
    for (int i = 0; i < varianceIntegratorLimit; i++) {
      int idx = varianceBufferIndex - i;
      if (idx < 0) idx += sampleSize;
      varianceIntegral += varianceBuffer[idx];
    }

    variance = varianceIntegral;
  }

  return variance;
}
