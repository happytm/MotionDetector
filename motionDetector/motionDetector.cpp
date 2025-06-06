#include "motionDetector.h"
#include <WiFi.h>  // THIS WILL MAKE THE LIBRARY WORK ONLY IN STA MODE OR AP_STA MODE AS LONG AS YOU'RE CONNECTED AS A STATION
#include "esp_wifi.h"   //LIBRARY WORK IN MULTISTATIC MODE IF YOU'VE GOT AT LEAST ONE STATION CONNECTED TO YOUR ESP32's SoftAP.

#define ENABLE_ALARM_THRESHOLD 0 // if 0, the raw variance signal is returned, if 1, only variance signals above threshold are returned, signals under threshold instead return zero or invalid (-1)

// internal variables and arrays

int enableThreshold = ENABLE_ALARM_THRESHOLD;

bool enableAutoRegressive = false;

#define MAX_SAMPLEBUFFERSIZE 256
int * sampleBuffer;
int sampleBufferSize = MAX_SAMPLEBUFFERSIZE; // default size, and maximum size  // the instant mobile average is calculated from here on mobileAverageFilterSize = n samples
int sampleBufferIndex = 0;
int sampleBufferValid = 0;

#define MAX_AVERAGEBUFFERSIZE 64
int mobileAverageFilterSize = MAX_SAMPLEBUFFERSIZE;
int mobileAverageBufferSize = MAX_AVERAGEBUFFERSIZE;
int mobileAverage = 0;
int mobileAverageTemp = 0;
int * mobileAverageBuffer; // the instant average value fills this circular buffer of size mobileAverageFilterSize with the mobileAverage values
int mobileAverageBufferIndex = 0;
int mobileAverageBufferValid = 0;

int bufferIndex = 0; // index used for all buffers. 

//// PLEASE NOTE: the init value is -1, meaning the variance is invalid, it will stay invalid until it can be effectively calculated
// DO NOT CHANGE THE -1 INITIALIZATION VALUE
int variance = RADAR_BOOTING; // this value is calculated from the current sample and the average calculated from the mobileAverageBuffer values

int variancePrev = 0;
int varianceSample = 0; // deviation of the current sample from the average
int varianceAR = 0;  // autoregressive version
int varianceIntegral = 0; 

#define MAX_VARIANCE 65535
int varianceThreshold = 3; // in variance arbitrary units
int varianceIntegratorLimitMax = MAX_SAMPLEBUFFERSIZE;
int varianceIntegratorLimit = 3;
int varianceBufferSize = MAX_SAMPLEBUFFERSIZE;
int *varianceBuffer; // holds the variance values
int detectionLevel = 0; // holds the detected level integrated from the varianceBuffer
int varianceBufferIndex = 0;
int varianceBufferValid = 0;
int enableCSVout = 0;
int minimumRSSI = MINIMUM_RSSI;


// functions

int motionDetector_init() {  // initializes the storage arrays in internal RAM

  if (sampleBuffer == NULL) {
    sampleBuffer =  (int*)malloc(sizeof(int) * sampleBufferSize); 
    for (int sbtempindex=0; sbtempindex<sampleBufferSize; sbtempindex++) {   // initialization to zero
      sampleBuffer[sbtempindex] = 0x00;
    }
  }
  if (mobileAverageBuffer == NULL) {
    mobileAverageBuffer =  (int*)malloc(sizeof(int) * mobileAverageBufferSize); 
    for (int mabtempindex=0; mabtempindex<mobileAverageBufferSize; mabtempindex++) {   // initialization to zero
      mobileAverageBuffer[mabtempindex] = 0x00;
    }  
  }
  if (varianceBuffer == NULL) {
    varianceBuffer =  (int*)malloc(sizeof(int) * varianceBufferSize); 
    for (int vartempindex=0; vartempindex<varianceBufferSize; vartempindex++) {   // initialization to zero
      varianceBuffer[vartempindex] = 0x00;
    }   
  }
  variance = -1; // to make it clear that the initial value MUST be -1


  return 1;

}

int motionDetector_init_PSRAM() {  // initializes the storage arrays in PSRAM (only for ESP32-CAM modules and ESP32-WROVER modules with the onboard SPI RAM chip)

  if (sampleBuffer == NULL) {
    sampleBuffer =  (int*)ps_malloc(sizeof(int) * sampleBufferSize); 
    for (int sbtempindex=0; sbtempindex<sampleBufferSize; sbtempindex++) {   // initialization to zero
      sampleBuffer[sbtempindex] = 0x00;
    }
  }
  if (mobileAverageBuffer == NULL) {
    mobileAverageBuffer =  (int*)ps_malloc(sizeof(int) * mobileAverageBufferSize); 
    for (int mabtempindex=0; mabtempindex<mobileAverageBufferSize; mabtempindex++) {   // initialization to zero
      mobileAverageBuffer[mabtempindex] = 0x00;
    }  
  }
  if (varianceBuffer == NULL) {
    varianceBuffer =  (int*)ps_malloc(sizeof(int) * varianceBufferSize); 
      for (int vartempindex=0; vartempindex<varianceBufferSize; vartempindex++) {   // initialization to zero
        varianceBuffer[vartempindex] = 0x00;
      }   
  }
  variance = -1; // to make it clear that the initial value MUST be -1

  return 1;

}


int motionDetector_deinit() {  // frees the storage arrays 

  if (sampleBuffer != NULL) {
    free(sampleBuffer);
  }

  if (mobileAverageBuffer != NULL) {
    free(mobileAverageBuffer);
  }
  
  if (varianceBuffer != NULL) {
    free(varianceBuffer);
  }

  return 1;

}


int motionDetector_config(int sampleBufSize = 256, int mobileAvgSize = 64, int varThreshold = 3, int varIntegratorLimit = 3, bool enableAR = false) {
  if (sampleBufSize >= MAX_SAMPLEBUFFERSIZE) {
    sampleBufSize = MAX_SAMPLEBUFFERSIZE;
  }
  sampleBufferSize = sampleBufSize;

  varianceBufferSize = sampleBufferSize;
  
  if (mobileAvgSize >= MAX_SAMPLEBUFFERSIZE) {
    mobileAvgSize = MAX_SAMPLEBUFFERSIZE;
  }
  mobileAverageFilterSize = mobileAvgSize;
  
  if (varThreshold >= MAX_VARIANCE) {
    varThreshold = MAX_VARIANCE;
  }
  varianceThreshold = varThreshold;

  if (varIntegratorLimit >= varianceIntegratorLimitMax) {
    varIntegratorLimit = varianceIntegratorLimitMax;
  }
  varianceIntegratorLimit = varIntegratorLimit;

  enableAutoRegressive = enableAR;

  return 1;

}


int motionDetector_process(int sample = 0) { // send the RSSI signal, returns the detection level ( < 0 -> error, == 0 -> no detection, > 0 -> detection level in variance arbitrary units)
  

  if ((sampleBuffer == NULL) || (mobileAverageBuffer == NULL) || (varianceBuffer == NULL)) {
 
    return RADAR_UNINITIALIZED; // unallocated buffers
  }

  if (sample < minimumRSSI) {
 
    return RADAR_RSSI_TOO_LOW;
  }

  sampleBuffer[sampleBufferIndex] = sample;
  sampleBufferIndex++;
  if ( sampleBufferIndex >= sampleBufferSize ) { // circular buffer, rewinding the index, if the buffer has been filled at least once, then we may start processing valid data
    sampleBufferIndex = 0;
    sampleBufferValid = 1;
  }
  
  if (sampleBufferValid >= 1) {
    // filling in the mobile average data buffer
    // the mobile average can be re-calculated even on a full sampleBufferSize set of valid samples, I see no problem in terms of computational load
    // calculating the current mobile average now.  the sampleBufferIndex points now to the oldest sample
    mobileAverageTemp = 0;
    int mobilePointer = 0;
    for (int mobileAverageSampleIndex = 0; mobileAverageSampleIndex < mobileAverageFilterSize; mobileAverageSampleIndex++) {
      mobilePointer = sampleBufferIndex - mobileAverageSampleIndex;
      if (mobilePointer <= 0) {
        mobilePointer = mobilePointer + (sampleBufferSize -1);
      }
      mobileAverageTemp = mobileAverageTemp + sampleBuffer[mobilePointer];
    }
    mobileAverage = mobileAverageTemp / mobileAverageFilterSize;
    // filling in the mobile average buffer with the fresh new value
    mobileAverageBuffer[mobileAverageBufferIndex] = mobileAverage;  // to be fair, this buffer is filled but still ...really unused.
    // truth be said, I'm filling the mobileAverageBuffer for future logging purposes. (TBD)
    
    // since we have the current mobile average data, we can also extract the current variance data. 
    // the variable named "variance" at this point still contains the *previous* value of the variance   
    variancePrev = variance;
    // deviation of the current sample
    varianceSample = (sample - mobileAverageBuffer[mobileAverageBufferIndex])*(sample - mobileAverageBuffer[mobileAverageBufferIndex]);
    // filling in the variance buffer
    varianceBuffer[varianceBufferIndex] = varianceSample;
    
    // the following is a mobile integrator filter that parses the circular buffer called varianceBuffer
    varianceIntegral = 0;
    int variancePointer = 0;
    for (int varianceBufferIndexTemp = 0; varianceBufferIndexTemp < varianceIntegratorLimit; varianceBufferIndexTemp++) {
     variancePointer = varianceBufferIndex - varianceBufferIndexTemp;
     if (variancePointer <=0) {
        variancePointer = variancePointer + (varianceBufferSize -1);
     }
     varianceIntegral = varianceIntegral + varianceBuffer[variancePointer]; // the full effect of this operation is to make the system more sensitive to continued variations of the RSSI, possibly meaning there's a moving object around the area.
    }
    // increasing and checking the variance buffer index
    varianceBufferIndex++;
    if ( varianceBufferIndex >= varianceBufferSize ) { // circular buffer, rewinding the index, if the buffer has been filled at least once, then we may start processing valid data
      varianceBufferIndex = 0;
      varianceBufferValid = 1; //please note we DO NOT need to have a fully validated buffer to work with the current M.A. data
    }
    // applying the autoregressive part
    varianceAR = (varianceIntegral + varianceAR) / 2; // the effect of this filter is to "smooth" down the signal over time, so it's a simple IIR (infinite impulse response) low pass filter. It makes the system less sensitive to noisy signals, especially those with a deviation of less than 1.

    // assigning the values according to the settings
    variance = varianceSample; 
    
    if (enableAutoRegressive) {
      variance = varianceAR;
    }
    if (! enableAutoRegressive) {
      variance = varianceIntegral;
    }
    
    // note: we needed to point to the current mobile average data for future operations, so we increase the MA buffer index only as the last step
    mobileAverageBufferIndex++;
    if ( mobileAverageBufferIndex >= mobileAverageBufferSize ) { // circular buffer, rewinding the index, if the buffer has been filled at least once, then we may start processing valid data
      mobileAverageBufferIndex = 0;
      mobileAverageBufferValid = 1; //please note we DO NOT need to have a fully validated buffer to work with the current M.A. data
    }
    
  }

  
  // final check to determine if the detected variance signal is above the detection threshold, this is only done if enableThreshold > 0 
  if ((variance >= varianceThreshold) && (enableThreshold > 0)) {
    detectionLevel = variance;
  
    return detectionLevel;
  }
  // variance signal under threshold, but otherwise valid?
  if ((variance < varianceThreshold) && (variance >= 0) && (enableThreshold > 0) ) {
    detectionLevel = 0;
 
    return detectionLevel;
  }
  
  return variance; // if the sample buffer is still invalid at startup, an invalid value is returned: -1, else the raw variance signal is returned
}






int motionDetector() { // request the RSSI level internally, then process the signal and return the detection level in variance arbitrary units

  int RSSIlevel = 0;
  int res = 0;
 
  RSSIlevel = (int)WiFi.RSSI();  // I know, this is lame, but will make this library work with most Arduino-supported devices

  // if the RSSI is zero, then we are most probably not connected to an upstream AP.

  if (RSSIlevel == 0) {
 
    return RADAR_INOPERABLE; // radar inoperable
  }
  
  res = motionDetector_process(RSSIlevel);
  
  return res;

}

// bistatic version ESP32 ONLY

#define SCANMODE_STA 0
#define SCANMODE_SOFTAP 1
#define SCANMODE_WIFIPROBE 2

int scanMode = SCANMODE_STA; // 0 = STA RSSI (SCANMODE_STA); 1 = SoftAP client scan (SCANMODE_SOFTAP); 2 = active wifi scan (SCANMODE_WIFIPROBE);

uint8_t strongestClientBSSID[6] = {0};
int strongestClientRSSI = -100;

int strongestClientfound = 0; // if found set to 1

int modeRes; // wifi mode reporting variable, initialized when radar operations are requested, and shared with other subfunctions

uint8_t BSSIDinUse[6] = {0}; // this array identifies the currently used BSSID


int bistatic_get_rssi_SoftAP_strongestClient() {
  int rssi = 0;
  //int scanRes = 0;

  uint8_t * currentBSSID;
  int currentRSSI = 0;
  wifi_sta_list_t stationList;
  esp_err_t scanRes = esp_wifi_ap_get_sta_list(&stationList);

  if (scanRes != ESP_OK) {
    return 0; 
  }

    // extracting the strongest signal (we need to mark it for subsequent scans, in order to build reliable stats) // this is executed one time when strongestClientfound == 0
  if (strongestClientfound == 0) {
    strongestClientRSSI = -100;
    for (int netItem = 0; netItem < stationList.num; netItem++) {
      wifi_sta_info_t station = stationList.sta[netItem];
      currentBSSID = station.mac;
      currentRSSI = station.rssi;
      
      if ((currentRSSI > -100) || (currentRSSI < 0)) {
        if (currentRSSI > strongestClientRSSI) {
          strongestClientfound = 1;
          strongestClientRSSI = currentRSSI;

          memcpy ( strongestClientBSSID, currentBSSID, (sizeof(uint8_t) * 6));

        }
      }
    }
  }

    int bssidScanOK = 0;
  if (strongestClientfound == 1) { // looking for the specific BSSID, if not found then reset strongestAPfound to zero and return an invalid RSSI set to 0
    for (int netItem = 0; netItem < stationList.num; netItem++) {  // parse all the network items found, look for the known strongest BSSID
      wifi_sta_info_t station = stationList.sta[netItem];
      currentBSSID = station.mac;
      for (int bssidIndex = 0; bssidIndex < 6; bssidIndex++) { // compare with the strongest BSSID, byte by byte
        if (currentBSSID[bssidIndex] == strongestClientBSSID[bssidIndex]) {
          bssidScanOK = 1; 
        } else {
          bssidScanOK = 0; // match failed, resetting the flag 
          break; // no matter wich single byte failed to match, we break out of the compare loop
        }
      }
      if (bssidScanOK == 1) {
        currentRSSI = station.rssi;

        rssi = currentRSSI; // preparing the final result
        break; // exit the scan loop
      }
    }
    if (bssidScanOK == 0) {
      strongestClientfound = 0;
      return 0; // lost the strongest BSSID, returning an invalid value
    }
  }
  
  if (strongestClientfound == 0) { // on no clients connected, we change scan mode (if possible)
    if (modeRes & WIFI_MODE_APSTA) {  
      if (WiFi.status() == WL_CONNECTED) {
        scanMode = SCANMODE_STA;
      } else {
        scanMode = SCANMODE_WIFIPROBE; // let's change scan mode
      }
    }
    rssi = 0; // in the meanwhile we return zero rssi
  }
  // returning the strongest signal

  
  memcpy ( BSSIDinUse, strongestClientBSSID, (sizeof(uint8_t) * 6)); // saving the current BSSID in use
  
  return rssi;
}

uint8_t strongestBSSID[6] = {0};
int strongestRSSI = -100;
int strongestChannel = 0;
int strongestAPfound = 0; // if found set to 1

int bistatic_get_rssi_ScanStrongestAP() {
  int rssi = 0;
  int scanRes = 0;

  uint8_t * currentBSSID;
  int currentRSSI = 0;
  int currentChannel = 0;


  if (strongestAPfound == 0) { // don't have a strongest AP on record yet? Do a slow full scan
    scanRes = (int) WiFi.scanNetworks(false, false, false, 300, 0); //scanNetworks(bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300, uint8_t channel = 0);
  } else { // do a single channel active scan, this should be much faster
    scanRes = (int) WiFi.scanNetworks(false, false, false, 200, strongestChannel); //scanNetworks(bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300, uint8_t channel = 0);
  }
 
  // extracting the strongest signal (we need to mark it for subsequent scans, in order to build reliable stats) // this is executed one time when strongestAPfound == 0
  if (strongestAPfound == 0) {
    strongestRSSI = -100;
    for (int netItem = 0; netItem < scanRes; netItem++) {
      currentBSSID = WiFi.BSSID(netItem);
      currentRSSI = WiFi.RSSI(netItem);
      currentChannel = WiFi.channel(netItem);

      if ((currentRSSI > -100) || (currentRSSI < 0)) {
        if (currentRSSI > strongestRSSI) {
          strongestAPfound = 1;
          strongestRSSI = currentRSSI;
          strongestChannel = currentChannel;
          memcpy ( strongestBSSID, currentBSSID, (sizeof(uint8_t) * 6));

        }
      }
    }
  }

  int bssidScanOK = 0;
  if (strongestAPfound == 1) { // looking for the specific BSSID, if not found then reset strongestAPfound to zero and return an invalid RSSI set to 0
    for (int netItem = 0; netItem < scanRes; netItem++) {  // parse all the network items found, look for the known strongest BSSID
      currentBSSID = WiFi.BSSID(netItem);  
      for (int bssidIndex = 0; bssidIndex < 6; bssidIndex++) { // compare with the strongest BSSID, byte by byte
        if (currentBSSID[bssidIndex] == strongestBSSID[bssidIndex]) {
          bssidScanOK = 1; 
        } else {
          bssidScanOK = 0; // match failed, resetting the flag 
          break; // no matter wich single byte failed to match, we break out of the compare loop
        }
      }
      if (bssidScanOK == 1) {
        currentRSSI = WiFi.RSSI(netItem);
        currentChannel = WiFi.channel(netItem);

        rssi = currentRSSI; // preparing the final result
        break; // exit the scan loop
      }
    }
    if (bssidScanOK == 0) {
      strongestAPfound = 0;
      strongestChannel = 0;
      WiFi.scanDelete();
      return 0; // lost the strongest BSSID, returning an invalid value
    }
  }

  if (strongestAPfound == 0) {
    strongestChannel = 0;
    rssi = 0;
    if (modeRes & WIFI_MODE_APSTA) {  
      scanMode = SCANMODE_SOFTAP; // let's change scan mode
    }
  }


  memcpy ( BSSIDinUse, strongestBSSID, (sizeof(uint8_t) * 6)); // saving the current BSSID in use
  
  // returning the strongest signal
  WiFi.scanDelete();
  
  return rssi;
}


void serialPrintBSSID(uint8_t * localBSSID) {
  for (int bssidIndex = 0; bssidIndex < 6; bssidIndex++) {
    if (localBSSID == NULL) {
      Serial.print("NULL");
      return;
    }
    
    Serial.print(localBSSID[bssidIndex], HEX);
  }
}


int motionDetector_esp() { // request the RSSI level internally, then process the signal and return the detection level in variance arbitrary units, self-detects faults and seeks for alternate solutions

  int RSSIlevel = 0;
  int res = 0;

  int scanRes = 0;

  modeRes = (int) WiFi.getMode();
  
  if (modeRes & WIFI_MODE_NULL) {
 
    return WIFI_MODEINVALID;
  }
  if ((modeRes & WIFI_MODE_APSTA) || (modeRes & WIFI_MODE_STA)) {
    RSSIlevel = (int)WiFi.RSSI();  // I know, this is lame, but will make this library work with most Arduino-supported devices
  }
  // if the RSSI is zero, then we are most probably not connected to an upstream AP.

  if (RSSIlevel == 0) {

    if ((modeRes & WIFI_MODE_APSTA) || (modeRes & WIFI_MODE_AP)) { // also SoftAP available
      // we first check for any connected clients, then scan if zero clients have been found
   
      if (scanMode == SCANMODE_SOFTAP) {

        RSSIlevel = bistatic_get_rssi_SoftAP_strongestClient();
      }

      if (scanMode == SCANMODE_WIFIPROBE) {

        RSSIlevel = bistatic_get_rssi_ScanStrongestAP();
      }
      
      if ((RSSIlevel == 0) && (scanMode == SCANMODE_SOFTAP)){ // SoftAP scan for connected clients failed, switching scan mode

        scanMode = SCANMODE_WIFIPROBE;
        RSSIlevel = bistatic_get_rssi_ScanStrongestAP();
      }

      if ((RSSIlevel == 0) && (scanMode == SCANMODE_WIFIPROBE)){ // WiFi probe scan for APs failed, switching scan mode

        scanMode = SCANMODE_SOFTAP;
        RSSIlevel = bistatic_get_rssi_SoftAP_strongestClient();
      }

      
      if (RSSIlevel == 0) { // also no APs around to be scanned

        scanMode == SCANMODE_SOFTAP; // it is still worth reverting to the most efficient scan mode.

      }
    }

    if (modeRes & WIFI_MODE_STA) { // STA only mode

      scanMode = SCANMODE_WIFIPROBE;
      if (scanMode == SCANMODE_WIFIPROBE) {
        RSSIlevel = bistatic_get_rssi_ScanStrongestAP();
      }      
      if (RSSIlevel == 0) {

      }
      
    }


    if (RSSIlevel == 0) { // also no APs around to be scanned

        scanMode == SCANMODE_SOFTAP; // it is still worth reverting to the most efficient scan mode.

        return RADAR_INOPERABLE; // radar inoperable
      }
}
  
  res = motionDetector_process(RSSIlevel); // the core operation won't change. 

  if (enableCSVout) {
    Serial.println("VarianceLevel");
    serialPrintBSSID(BSSIDinUse);
    Serial.print("_");
    Serial.println(RSSIlevel);
    Serial.println(res);
  }
  
  return res;

}

int motionDetector_enable_serial_CSV_graph_data(int serialCSVen = 0) {

  if (serialCSVen >= 0) {
    enableCSVout = serialCSVen;
  }
  if (serialCSVen > 0) {
    //motionDetector_set_debug_level(0);
  }
  
  return serialCSVen;
  
}

int motionDetector_set_minimum_RSSI(int rssiMin) {

  if ((rssiMin > 0) || (rssiMin < ABSOLUTE_RSSI_LIMIT)) {
    rssiMin == ABSOLUTE_RSSI_LIMIT; // which results in disabling the minimum RSSI check
  }
    
    minimumRSSI = rssiMin;
    return rssiMin;
}


// current status: IMPLEMENTED
int motionDetector_enable_alarm(int thresholdEnable) {
  if (thresholdEnable < 0) {
    thresholdEnable = 0; // which results in disabling the alarm
  }

  
  enableThreshold = thresholdEnable; // Lol!
  
  return enableThreshold;
  
}


// current status: IMPLEMENTED
int motionDetector_set_alarm_threshold(int alarmThreshold) {
    if (alarmThreshold < 0) {
    alarmThreshold = 0; // which results in always enabling the alarm
  }

  

  varianceThreshold = alarmThreshold;
  
  return alarmThreshold;
}
