#include <WiFi.h>
#include "esp_wifi.h"

// Constants and Macros
#define MAX_SAMPLEBUFFERSIZE 256
#define MAX_AVERAGEBUFFERSIZE 64
#define MAX_VARIANCE 65535
#define ENABLE_ALARM_THRESHOLD 0
#define RADAR_BOOTING -1
#define RADAR_UNINITIALIZED -2
#define RADAR_RSSI_TOO_LOW -3
#define RADAR_INOPERABLE -4
#define WIFI_MODEINVALID -5
#define ABSOLUTE_RSSI_LIMIT -100

// Global variables (only include what's necessary for your use)
int enableThreshold = ENABLE_ALARM_THRESHOLD;
bool enableAutoRegressive = false;
int *sampleBuffer = nullptr;
int *mobileAverageBuffer = nullptr;
int *varianceBuffer = nullptr;

int sampleBufferSize = MAX_SAMPLEBUFFERSIZE;
int mobileAverageFilterSize = MAX_SAMPLEBUFFERSIZE;
int mobileAverageBufferSize = MAX_AVERAGEBUFFERSIZE;
int mobileAverage = 0;
int mobileAverageTemp = 0;
int sampleBufferIndex = 0;
int sampleBufferValid = 0;
int mobileAverageBufferIndex = 0;
int mobileAverageBufferValid = 0;
int variance = RADAR_BOOTING;
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

int motionDetector_config(
  int sampleBufSize = 256,
  int mobileAvgSize = 64,
  int varThreshold = 3,
  int varIntegratorLimit = 3,
  bool enableAR = false
) {
  if (sampleBufSize > MAX_SAMPLEBUFFERSIZE) {
    sampleBufSize = MAX_SAMPLEBUFFERSIZE;
  }
  sampleBufferSize = sampleBufSize;
  varianceBufferSize = sampleBufSize;

  if (mobileAvgSize > MAX_SAMPLEBUFFERSIZE) {
    mobileAvgSize = MAX_SAMPLEBUFFERSIZE;
  }
  mobileAverageFilterSize = mobileAvgSize;

  if (varThreshold > MAX_VARIANCE) {
    varThreshold = MAX_VARIANCE;
  }
  varianceThreshold = varThreshold;

  if (varIntegratorLimit > varianceIntegratorLimitMax) {
    varIntegratorLimit = varianceIntegratorLimitMax;
  }
  varianceIntegratorLimit = varIntegratorLimit;

  enableAutoRegressive = enableAR;

  return 1;
}


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin("ESP", "");

  // Wait for WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");

  // Configure motion detector
  motionDetector_config(128, 32, 5, 5, true); // Example custom settings

  // Allocate memory based on config
  sampleBuffer = (int*)malloc(sizeof(int) * sampleBufferSize);
  mobileAverageBuffer = (int*)malloc(sizeof(int) * mobileAverageBufferSize);
  varianceBuffer = (int*)malloc(sizeof(int) * varianceBufferSize);
  for (int i = 0; i < sampleBufferSize; i++) sampleBuffer[i] = 0;
  for (int i = 0; i < mobileAverageBufferSize; i++) mobileAverageBuffer[i] = 0;
  for (int i = 0; i < varianceBufferSize; i++) varianceBuffer[i] = 0;
}


void loop() {
  int rssi = WiFi.RSSI();
  int result = motionDetector_process(rssi);
  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.print(" -> Detection: ");
  Serial.println(result);
  delay(100); // Adjust as needed
}

int motionDetector_process(int sample) {
  if (!sampleBuffer || !mobileAverageBuffer || !varianceBuffer)
    return RADAR_UNINITIALIZED;
  if (sample < minimumRSSI)
    return RADAR_RSSI_TOO_LOW;

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
    mobileAverageBuffer[mobileAverageBufferIndex] = mobileAverage;

    variancePrev = variance;
    varianceSample = (sample - mobileAverageBuffer[mobileAverageBufferIndex]) *
                     (sample - mobileAverageBuffer[mobileAverageBufferIndex]);
    varianceBuffer[varianceBufferIndex] = varianceSample;

    varianceIntegral = 0;
    for (int i = 0; i < varianceIntegratorLimit; i++) {
      int idx = varianceBufferIndex - i;
      if (idx < 0) idx += varianceBufferSize;
      varianceIntegral += varianceBuffer[idx];
    }

    varianceBufferIndex++;
    if (varianceBufferIndex >= varianceBufferSize) {
      varianceBufferIndex = 0;
      varianceBufferValid = 1;
    }

    varianceAR = (varianceIntegral + varianceAR) / 2;

    if (enableAutoRegressive)
      variance = varianceAR;
    else
      variance = varianceIntegral;

    mobileAverageBufferIndex++;
    if (mobileAverageBufferIndex >= mobileAverageBufferSize) {
      mobileAverageBufferIndex = 0;
      mobileAverageBufferValid = 1;
    }
  }

  if (enableThreshold && variance >= varianceThreshold)
    return variance;
  if (enableThreshold && variance < varianceThreshold && variance >= 0)
    return 0;

  return variance;
}
