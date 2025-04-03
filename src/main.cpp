#include "config.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <NimBLEDevice.h>
#include <map>
#include <vector>

WiFiClient espClient;
PubSubClient mqttClient(espClient);
DHT dht(DHTPIN, DHTTYPE);

struct DeviceInfo {
  int8_t rssi;
};

struct Snapshot {
  unsigned long timestamp;
  float temp;
  float humidity;
  std::map<String, DeviceInfo> wifi;
  std::map<String, DeviceInfo> bt;
};

std::vector<Snapshot> bufferedData;
std::map<String, DeviceInfo> currentWiFi;
std::map<String, DeviceInfo> currentBT;

NimBLEScan* pBLEScan;
unsigned long lastSample = 0;

uint8_t currentChannel = 1;
unsigned long lastChannelSwitch = 0;
const unsigned long CHANNEL_SWITCH_INTERVAL = 3000;

// IEEE 802.11 header struct
typedef struct {
  uint8_t frame_ctrl[2];
  uint8_t duration_id[2];
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  uint8_t seq_ctrl[2];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[];
} wifi_ieee80211_packet_t;

void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
  const wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

  const uint8_t* mac = hdr->addr2;

  // Hvis addr2 er NULL eller 00:00:00:00:00:00, prøv addr3
  bool invalid = true;
  for (int i = 0; i < 6; i++) {
    if (mac[i] != 0x00) {
      invalid = false;
      break;
    }
  }
  if (invalid) mac = hdr->addr3;

  // Endnu en fallback (addr1)
  if (invalid) mac = hdr->addr1;

  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  String macKey(macStr);

  if (currentWiFi.find(macKey) == currentWiFi.end()) {
    currentWiFi[macKey] = { static_cast<int8_t>(pkt->rx_ctrl.rssi) };
  }
}

class BTAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* dev) override {
    std::string mac = dev->getAddress().toString();
    int8_t rssi = dev->getRSSI();

    if (currentBT.find(mac.c_str()) == currentBT.end()) {
      currentBT[mac.c_str()] = { rssi };
    }
  }
};

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (ENABLE_SERIAL) Serial.print(".");
  }
  if (ENABLE_SERIAL) Serial.println("\n✅ WiFi connected");
}

void reconnectMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  while (!mqttClient.connected()) {
    if (ENABLE_SERIAL) Serial.print("📡 Connecting to MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      if (ENABLE_SERIAL) Serial.println(" ✅");
    } else {
      if (ENABLE_SERIAL) Serial.printf(" ❌ (%d)\n", mqttClient.state());
      delay(2000);
    }
  }
}

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
  for (size_t i = 0; i < bufferedData.size(); ++i) {
    out += snapshotToJson(bufferedData[i]);
    if (i < bufferedData.size() - 1) out += ",";
  }
  out += "]";
  return out;
}

void blinkGreenLED() {
  digitalWrite(GREEN_LED, HIGH);
  delay(200);
  digitalWrite(GREEN_LED, LOW);
}

void blinkRedLED(int times = 3) {
  for (int i = 0; i < times; i++) {
    digitalWrite(RED_LED, HIGH);
    delay(200);
    digitalWrite(RED_LED, LOW);
    delay(200);
  }
}

void cycleWiFiChannel() {
  currentChannel = (currentChannel % 13) + 1;
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
  if (ENABLE_SERIAL) Serial.printf("Skifter til kanal: %d\n", currentChannel);
}

void setup() {
  if (ENABLE_SERIAL) Serial.begin(115200);
  delay(1000);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  dht.begin();

  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new BTAdvertisedDeviceCallbacks(), false);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(45);
  pBLEScan->setWindow(15);

  WiFi.mode(WIFI_MODE_NULL);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&snifferCallback);

  lastSample = millis();
}

void loop() {
  unsigned long now = millis();

  if (now - lastChannelSwitch >= CHANNEL_SWITCH_INTERVAL) {
    cycleWiFiChannel();
    lastChannelSwitch = now;
  }

  if (now - lastSample >= SAMPLE_INTERVAL_MS) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      if (ENABLE_SERIAL) Serial.println("❌ Sensorfejl – rød LED blinker");
      blinkRedLED();
      return;
    }

    blinkGreenLED();

    currentBT.clear();
    pBLEScan->start(BT_SCAN_DURATION_SEC, false);
    pBLEScan->clearResults();

    Snapshot snap;
    snap.timestamp = now;
    snap.temp = t;
    snap.humidity = h;
    snap.wifi = currentWiFi;
    snap.bt = currentBT;

    bufferedData.push_back(snap);
    currentWiFi.clear();
    lastSample = now;

    if (ENABLE_SERIAL) {
      Serial.printf("\n📦 Sample %d/%d buffered\n", bufferedData.size(), BUFFER_LIMIT);
      Serial.printf("   WiFi devices found: %d\n", (int)snap.wifi.size());
      Serial.printf("   BT devices found  : %d\n", (int)snap.bt.size());
      Serial.printf("   Sniffed on channel: %d\n", currentChannel);
    }
  }

  if (bufferedData.size() >= BUFFER_LIMIT) {
    if (ENABLE_SERIAL) Serial.println("🚀 Buffer fuld! Skifter til WiFi...");

    esp_wifi_set_promiscuous(false);
    WiFi.disconnect(true);
    delay(100);

    connectToWiFi();
    reconnectMQTT();

    String payload = exportAllSnapshotsAsJson();

    if (ENABLE_SERIAL) {
      Serial.println("📤 Klar til MQTT:");
      Serial.println(payload);
    }

    mqttClient.publish(MQTT_TOPIC, payload.c_str());

    bufferedData.clear();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_MODE_NULL);
    esp_wifi_set_promiscuous(true);
    delay(100);
  }

  mqttClient.loop();
}