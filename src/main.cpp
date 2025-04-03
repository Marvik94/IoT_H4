#include <FS.h>
#include <LittleFS.h>
#include "config.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <NimBLEDevice.h>
#include <map>
#include <vector>

// ========== WiFi / MQTT ==========
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ========== DHT Sensor ==========
DHT dht(DHTPIN, DHTTYPE);

// ========== Strukturer ==========
struct DeviceInfo {
  int8_t rssi;
};

struct Snapshot {
  unsigned long timestamp = 0;
  float temp = NAN;
  float humidity = NAN;
  std::map<String, DeviceInfo> wifi;
  std::map<String, DeviceInfo> bt;
};

std::vector<Snapshot> bufferedData;
std::map<String, DeviceInfo> currentWiFi;
std::map<String, DeviceInfo> currentBT;

NimBLEScan* pBLEScan;
unsigned long lastSample = 0;

// Kanalhop-variabler
uint8_t currentChannel = 1;
unsigned long lastChannelSwitch = 0;
const unsigned long CHANNEL_SWITCH_INTERVAL = 3000; // 3 sek

// ========== 802.11 Structs ==========
// Minimal version til at få 'addr2'
typedef struct {
  uint8_t frame_ctrl[2];
  uint8_t duration_id[2];
  uint8_t addr1[6];
  uint8_t addr2[6];  // Transmitter
  uint8_t addr3[6];  // AP/STA?
  uint8_t seq_ctrl[2];
  // evt. addr4 hvis du vil have QoS, men skip
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[]; 
} wifi_ieee80211_packet_t;

// ========== Sniffer Callback ==========
void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  // Kast den rå buffer til en promiscuous-pakke
  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*) buf;

  // Kast payload til en 802.11-pakke, så vi kan læse header
  const wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*) pkt->payload;
  const wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

  // Vi tager 'addr2' = transmitter
  const uint8_t* mac = hdr->addr2;

  // Tjek for edge-case: er mac 00:00:00...?
  bool allZero = true;
  for(int i=0; i<6; i++){
    if(mac[i] != 0){
      allZero = false;
      break;
    }
  }
  // fallback: hvis allZero, brug 'addr3'? 
  if(allZero) mac = hdr->addr3;

  // Byg streng
  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  String macKey(macStr);

  // Gem kun unikke MAC. RSSI ligger i pkt->rx_ctrl.rssi
  if(currentWiFi.find(macKey) == currentWiFi.end()) {
    currentWiFi[macKey] = { (int8_t) pkt->rx_ctrl.rssi };
  }
}

// ========== Bluetooth Scan Callback ==========
class BTAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* dev) override {
    std::string mac = dev->getAddress().toString();
    int8_t rssi = dev->getRSSI();
    if(currentBT.find(mac.c_str()) == currentBT.end()) {
      currentBT[mac.c_str()] = { rssi };
    }
  }
};

// ========== WiFi + MQTT ==========
void connectToWiFi() {
  // Sæt STA-mode, forbinder
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    if(ENABLE_SERIAL) Serial.print(".");
  }
  if(ENABLE_SERIAL) Serial.println("\n✅ WiFi connected");
}

void reconnectMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  while(!mqttClient.connected()) {
    if(ENABLE_SERIAL) Serial.print("📡 Connecting to MQTT...");
    if(mqttClient.connect(MQTT_CLIENT_ID)) {
      if(ENABLE_SERIAL) Serial.println(" ✅");
    } else {
      if(ENABLE_SERIAL) Serial.printf(" ❌ (%d)\n", mqttClient.state());
      delay(2000);
    }
  }
}

// ========== JSON Export ==========
String snapshotToJson(const Snapshot& s) {
  String json = "{";
  json += "\"timestamp\":" + String(s.timestamp) + ",";
  json += "\"temp\":" + String(s.temp, 1) + ",";
  json += "\"humidity\":" + String(s.humidity, 1) + ",";
  json += "\"wifiCount\":" + String(s.wifi.size()) + ",";
  json += "\"btCount\":" + String(s.bt.size());
  json += "}";
  return json;
}

String exportAllSnapshotsAsJson() {
  String out = "[";
  for(size_t i=0; i<bufferedData.size(); i++){
    out += snapshotToJson(bufferedData[i]);
    if(i < bufferedData.size()-1) out += ",";
  }
  out += "]";
  return out;
}

// ========== LED Helpers ==========
void blinkGreenLED() {
  digitalWrite(GREEN_LED, HIGH);
  delay(200);
  digitalWrite(GREEN_LED, LOW);
}

void blinkRedLED(int times=3) {
  for(int i=0; i<times; i++){
    digitalWrite(RED_LED, HIGH);
    delay(200);
    digitalWrite(RED_LED, LOW);
    delay(200);
  }
}

// ========== Kanal-hop ==========
void cycleWiFiChannel() {
  currentChannel = (currentChannel % 13) + 1;
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
  if(ENABLE_SERIAL) Serial.printf("Skifter til kanal: %d\n", currentChannel);
}

// ========== Offline Data ==========
void storeOffline(const String& payload) {
  File f = LittleFS.open("/cache.txt", FILE_APPEND);
  if(!f){
    Serial.println("Kan ikke åbne /cache.txt til append!");
    return;
  }
  f.println(payload); // hver linje = et JSON
  f.close();
  Serial.println("Offline data gemt på /cache.txt");
}

// ========== Send Offline Data ==========
// Send offline-data fra fil, hvis der er nogen
void sendOfflineData() {
  File f = LittleFS.open("/cache.txt", FILE_READ);
  if(!f){
    Serial.println("Ingen offline-data at sende.");
    return;
  }
  while(f.available()){
    String line = f.readStringUntil('\n');
    line.trim();
    if(line.length() > 0){
      Serial.println("Sender offline-linje: " + line);
      mqttClient.publish(MQTT_TOPIC, line.c_str());
      delay(100); // lille pause
    }
  }
  f.close();

  // Ryd filen efter vellykket send
  LittleFS.remove("/cache.txt");
  Serial.println("Offline data er nu sendt og fil slettet.");
}

// ========== Setup ==========
void setup() {
  if(ENABLE_SERIAL) Serial.begin(115200);
  delay(1000);
  
  if(!LittleFS.begin(true)){ // 'true' = format on fail
    Serial.println("LittleFS mount failed!");
  } else {
    Serial.println("LittleFS ready.");
  }

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  dht.begin();

  // Start BT scanning
  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new BTAdvertisedDeviceCallbacks(), false);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(45);
  pBLEScan->setWindow(15);

  // Start i Null-mode (ingen WiFi-forbindelse)
  WiFi.mode(WIFI_MODE_NULL);

  // Gør Esp i promiscuous + callback
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&snifferCallback);

  lastSample = millis();
}

// ========== Loop ==========
void loop() {
  unsigned long now = millis();

  // Skift kanal hver 3 sek
  if(now - lastChannelSwitch >= CHANNEL_SWITCH_INTERVAL) {
    cycleWiFiChannel();
    lastChannelSwitch = now;
  }

  // Tag en måling hvert SAMPLE_INTERVAL_MS (def. i config.h)
  if(now - lastSample >= SAMPLE_INTERVAL_MS) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if(isnan(t) || isnan(h)) {
      if(ENABLE_SERIAL) Serial.println("Sensorfejl – rød LED blinker");
      blinkRedLED();
      return;
    }

    // Blink LED for at indikere sample
    blinkGreenLED();

    // Kør en kort BT-scan
    currentBT.clear();
    pBLEScan->start(BT_SCAN_DURATION_SEC, false);
    pBLEScan->clearResults();

    // Opbyg snapshot
    Snapshot snap;
    snap.timestamp = now;
    snap.temp = t;
    snap.humidity = h;
    snap.wifi = currentWiFi;
    snap.bt = currentBT;

    bufferedData.push_back(snap);
    currentWiFi.clear();
    lastSample = now;

    if(ENABLE_SERIAL) {
      Serial.printf("\nSample %d/%d buffered\n",
                    bufferedData.size(), BUFFER_LIMIT);
      Serial.printf("   WiFi devices found: %d\n", (int)snap.wifi.size());
      Serial.printf("   BT devices found  : %d\n", (int)snap.bt.size());
      Serial.printf("   Sniffed on channel: %d\n", currentChannel);
    }
  }

  // Hvis buffer er fyldt, skift til WiFi, send data, og gå tilbage
  if (bufferedData.size() >= BUFFER_LIMIT) {
    esp_wifi_set_promiscuous(false);
    WiFi.disconnect(true);
    delay(100);
  
    connectToWiFi();
  
    String payload = exportAllSnapshotsAsJson(); // dine data
  
    if (WiFi.status() == WL_CONNECTED) {
      reconnectMQTT();
      if (mqttClient.connected()) {
        // Først: send offline-data
        sendOfflineData();
  
        // Dernæst: send ny data
        mqttClient.publish(MQTT_TOPIC, payload.c_str());
        Serial.println("Ny data sendt via MQTT.");
      } else {
        // kunne ikke forbinde til MQTT → gem i fil
        storeOffline(payload);
      }
    } else {
      // WiFi fejler → gem i fil
      storeOffline(payload);
    }
  
    bufferedData.clear();
  
    // tilbage i promiscuous
    WiFi.disconnect(true);
    WiFi.mode(WIFI_MODE_NULL);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
    delay(100);
  }  
}
// ========== End of file ==========