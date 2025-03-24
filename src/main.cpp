#include <WiFi.h>
#include <DHT.h>
#include <NimBLEDevice.h>

// WiFi login
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

class BTAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    btDeviceCount++;
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

  // Init Bluetooth scanning
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

    // Scan Bluetooth i 5 sekunder
    btDeviceCount = 0;
    pBLEScan->start(5, false);
    pBLEScan->clearResults(); // frigiv hukommelse

    // Print resultater
    Serial.printf("🌡 Temp: %.1f °C  💧 Fugt: %.1f %%\n", temperature, humidity);
    Serial.printf("📶 WiFi enheder: %d\n", wifiCount);
    Serial.printf("🔵 Bluetooth enheder: %d\n", btDeviceCount);
  }

  delay(10000);
}
  
// The code is pretty simple. It connects to WiFi, reads the temperature and humidity from a DHT11 sensor, scans for WiFi networks, and scans for Bluetooth devices. 
// The code is available on GitHub: