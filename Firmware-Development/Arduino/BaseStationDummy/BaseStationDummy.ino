/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE"
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <time.h>
#include "cJSON.h"

#define NO_OF_DEVICES 5
#define NO_OF_REGIONS NO_OF_DEVICES
#define BASE_TIME     1743051579   // UNIX-Epoch time, Thu Mar 27 2025 04:59:39 GMT+0000
#define BASE_TIME_SS  15
#define BASE_TIME_MM  59
#define BASE_TIME_HH  14

#define DELAY_TIME    3            // minutes

struct Timestamp {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct GeoCoordinates {
    double latitude;
    double longitude;
};

struct CommPacket {
    uint32_t ids[3];
    Timestamp timestamp;
    GeoCoordinates geoCoordinates;
}DevicePacket;

uint32_t DeviceIDs[NO_OF_DEVICES][3] = {  {1001, 1002, 1003},
                                          {2001, 2002, 2003},
                                          {3001, 3002, 3003},
                                          {4001, 4002, 4003},
                                          {5001, 5002, 5003},
                                        };

double LocationCoordinates[NO_OF_REGIONS][5][2] = {
  // Region 1  (PK)
  { {24.8607, 67.0011}, // Karachi
    {31.5497, 74.3436}, // Lahore
    {33.6844, 73.0479}, // Islamabad
    {30.1878, 66.9939}, // Quetta
    {34.0222, 71.5243}, // Peshawar
  },
  // Region 2  (UK)
  { {51.5074, 0.1278},   // London
    {52.4960, -1.5479},  // Birmingham
    {53.5357, -1.4821},  // Manchester
    {50.3932, -3.4903},  // Plymouth
    {55.9299, -3.0130},  // Edinburgh
  },
  // Region 3  (US)
  { {40.7128, -74.0060},  // New York
    {34.0522, -118.2437}, // Los Angeles
    {41.8781, -87.6298},  // Chicago
    {29.7604, -95.3698},  // Houston
    {37.7749, -122.4194}, // San Francisco
  },
  // Region 4  (KSA)
  { {24.5285, 39.3149}, // Madinah
    {24.7136, 46.6753}, // Riyadh
    {21.4858, 39.1925}, // Jeddah
    {26.4207, 50.0888}, // Dammam
    {21.3891, 39.8579}, // Mecca
  },
  // Region 5  (BRA)
  { {-15.9839, -46.0976}, // Brasillia
    {-23.5505, -46.6333}, // Sao Paulo
    {-22.9068, -43.1729}, // Rio de Janeiro
    {-3.7172, -38.5433},  // Manaus
    {-15.7942, -47.8825}, // Goiania
  },
};

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
uint64_t timeStamp = 0;
bool firstExecution = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    firstExecution = true;
    timeStamp = millis();
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }

      Serial.println();
      Serial.println("*********");
    }
  }
};

void formatTimestamp(uint64_t unix_time, char *formatted_time, size_t size) {
  time_t raw_time = (time_t)unix_time;
  struct tm timeinfo;
  
  localtime_r(&raw_time, &timeinfo);
  
  snprintf(formatted_time, size, "20%02d-%02d-%02d  %02d:%02d:%02d",
           timeinfo.tm_year % 100, timeinfo.tm_mon + 1, timeinfo.tm_mday,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

char* convertToJson(struct CommPacket *packet) {
  cJSON *json = cJSON_CreateObject();
  
  cJSON *idsArray = cJSON_CreateArray();
  for (int i = 0; i < 3; i++) {
      cJSON_AddItemToArray(idsArray, cJSON_CreateNumber(packet->ids[i]));
  }
  cJSON_AddItemToObject(json, "ids", idsArray);

  // char formatted_time[30];  
  // formatTimestamp(packet->timestamp, formatted_time, sizeof(formatted_time));
  // cJSON_AddStringToObject(json, "timestamp", formatted_time);
  cJSON *timestamp = cJSON_CreateObject();
  cJSON_AddNumberToObject(timestamp, "hour", packet->timestamp.hour);
  cJSON_AddNumberToObject(timestamp, "minute", packet->timestamp.minute);
  cJSON_AddNumberToObject(timestamp, "second", packet->timestamp.second);
  cJSON_AddItemToObject(json, "timestamp", timestamp);

  cJSON *geo = cJSON_CreateObject();
  cJSON_AddNumberToObject(geo, "latitude", packet->geoCoordinates.latitude);
  cJSON_AddNumberToObject(geo, "longitude", packet->geoCoordinates.longitude);
  cJSON_AddItemToObject(json, "geoCoordinates", geo);

  char *jsonString = cJSON_Print(json);
  cJSON_Delete(json);
  return jsonString;
}

void generatePacket(int deviceNo) {
  DevicePacket.ids[0] = DeviceIDs[deviceNo][0];
  DevicePacket.ids[1] = DeviceIDs[deviceNo][1];
  DevicePacket.ids[2] = DeviceIDs[deviceNo][2];

  int64_t microseconds = esp_timer_get_time();
  int seconds = microseconds / 1000000;
  // DevicePacket.timestamp =  1743051579 + seconds;
  DevicePacket.timestamp.second = (BASE_TIME_SS + seconds) % 60;
  DevicePacket.timestamp.minute = (((BASE_TIME_SS + seconds)/60) + BASE_TIME_MM) % 60;
  DevicePacket.timestamp.hour =   (((((BASE_TIME_SS + seconds)/60) + BASE_TIME_MM)/60) + BASE_TIME_HH) % 24;

  uint8_t areaIndex = (txValue % NO_OF_DEVICES);
  DevicePacket.geoCoordinates.latitude = LocationCoordinates[deviceNo][areaIndex][0];
  DevicePacket.geoCoordinates.longitude = LocationCoordinates[deviceNo][areaIndex][1];

  char *jsonStr = convertToJson(&DevicePacket);
  Serial.println(jsonStr);
  free(jsonStr);
}

void sendPackets() {
  for (int i = 0; i < NO_OF_DEVICES; i++) {
    generatePacket(i);
    pTxCharacteristic->setValue((uint8_t*)&DevicePacket, sizeof(DevicePacket));
    pTxCharacteristic->notify();
    delay(100);
  }
  txValue++;
}

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  if (deviceConnected) {
    unsigned long currentMillis = millis();
    
    // Check if it's the first execution
    if (firstExecution) {
      if (currentMillis - timeStamp >= 5000) { // 5-second initial delay for allowing client to subscribe
        Serial.println("Generating packets and transmitting over BLE.");
        sendPackets();
        timeStamp = currentMillis;
        firstExecution = false; // Reset first execution flag
      }
    }
    // Subsequent executions based on DELAY_TIME
    else if (currentMillis - timeStamp >= (DELAY_TIME * 60 * 1000)) {
      Serial.println("Generating packets and transmitting over BLE.");
      sendPackets();
      timeStamp = currentMillis;
    }
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
