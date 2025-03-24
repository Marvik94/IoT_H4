#include <WiFi.h>
#include <DHT.h>
#include <NimBLEDevice.h>

// WiFi credentials
const char* ssid = "MD iPhone";
const char* password = "mtest001_";

// DHT setup
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LED pins
#define GREEN_LED 17
#define RED_LED 16

int btDeviceCount = 0;

struct BTDeviceInfo {
  std::string name;
  std::string address;
  int rssi;
};

std::vector<BTDeviceInfo> btDevices;

class BTAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    btDeviceCount++;
    BTDeviceInfo dev;
    dev.name = advertisedDevice->getName().length() > 0 ? advertisedDevice->getName() : "(ukendt)";
    dev.address = advertisedDevice->getAddress().toString();
    dev.rssi = advertisedDevice->getRSSI();
    btDevices.push_back(dev);
  }
};

NimBLEScan* pBLEScan;

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  dht.begin();
  delay(2000);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  // Init Bluetooth scanner
  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new BTAdvertisedDeviceCallbacks(), false);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(45);
  pBLEScan->setWindow(15);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("❌ Sensorfejl – rød LED blinker");
    for (int i = 0; i < 3; i++) {
      digitalWrite(RED_LED, HIGH);
      delay(200);
      digitalWrite(RED_LED, LOW);
      delay(200);
    }
  } else {
    // Grøn LED blink
    digitalWrite(GREEN_LED, HIGH);
    delay(200);
    digitalWrite(GREEN_LED, LOW);

    // Scan WiFi
    int wifiCount = WiFi.scanNetworks();
    Serial.printf("🌡 Temp: %.1f °C  💧 Fugt: %.1f %%\n", temperature, humidity);
    Serial.printf("📶 WiFi enheder: %d\n", wifiCount);

    for (int i = 0; i < wifiCount; ++i) {
      Serial.printf("  [%d] SSID: %s, MAC: %s, RSSI: %d dBm, Channel: %d\n",
        i + 1,
        WiFi.SSID(i).c_str(),
        WiFi.BSSIDstr(i).c_str(),
        WiFi.RSSI(i),
        WiFi.channel(i));
    }

    // Scan Bluetooth
    btDevices.clear();
    btDeviceCount = 0;
    pBLEScan->start(5, false);
    pBLEScan->clearResults();

    Serial.printf("🔵 Bluetooth enheder: %d\n", btDeviceCount);
    for (size_t i = 0; i < btDevices.size(); ++i) {
      Serial.printf("  [%d] Navn: %s, MAC: %s, RSSI: %d dBm\n",
        (int)(i + 1),
        btDevices[i].name.c_str(),
        btDevices[i].address.c_str(),
        btDevices[i].rssi);
    }
  }

  delay(10000);
}
  
//  The code is pretty straightforward. It connects to WiFi, reads the temperature and humidity from a DHT11 sensor, and then scans for WiFi and Bluetooth devices. 
//  The Bluetooth scanning is done using the NimBLE library, which is a lightweight Bluetooth Low Energy (BLE) library for ESP32. 
//  The code is also available on GitHub: