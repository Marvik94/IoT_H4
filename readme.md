# IoT Persontælling & Indeklima -- ESP32 Projekt

Dette projekt bruger en **ESP32** til at:

- **Tælle enheder** i lokalet via **WiFi-sniffing** (2.4 GHz) og **Bluetooth** (BLE)\
- **Måle temperatur og luftfugtighed** via en DHT-sensor (eks. DHT11)\
- **Buffer og offline-lagre data** (LittleFS), hvis netforbindelse mangler\
- **Sende data** som JSON via **MQTT** efter et antal målinger

Derudover vises status med LED'er:

- ✅ Grøn LED blinker ved succesfuld måling\
- ❌ Rød LED blinker ved sensorfejl

> **Vigtig note:** ESP32 kan kun scanne **2.4 GHz** -- hvis rummet primært bruger 5 GHz WiFi, vil `wifiCount` ofte være 0.

---

## ESP32 Opsætning

Projektet er udviklet med [PlatformIO](https://platformio.org/) og kræver følgende:

### 1. Installer PlatformIO

- Installer som plugin til [Visual Studio Code](https://code.visualstudio.com/)\
- Åbn projektmappen i VS Code

### 2. Tilpas `config.h`

Find og redigér `src/config.h`:

```cpp\
#define WIFI_SSID "DIT_WIFI_NAVN"\
#define WIFI_PASS "DIN_WIFI_KODE"

#define MQTT_SERVER "DIN_PC_IP"\
#define MQTT_PORT 1884\
#define MQTT_CLIENT_ID "esp32_sniffer"\
#define MQTT_TOPIC "sniffer/data"

// Andre parametre som SAMPLE_INTERVAL_MS, BT_SCAN_DURATION_SEC, BUFFER_LIMIT, etc.

Tip: Find din PC's IP ved at skrive ipconfig i en terminal.\
Lokal MQTT Broker (Mosquitto)

Projektet bruger Mosquitto som MQTT-broker. En færdig konfigurationsfil (mosquitto.conf) er inkluderet.\
Trin 1: Installer Mosquitto

Download og installer fra:\
https://mosquitto.org/download/

Sørg for at tilføje Mosquitto til systemets PATH under installationen (eller gør det manuelt bagefter).\
Trin 2: Start broker med konfig

I projektmappen (hvor mosquitto.conf ligger), kør:

mosquitto -c ./mosquitto.conf -v

Dette starter en broker på port 1884 og tillader anonyme forbindelser.\
Trin 3: Subscribe til ESP32-data

mosquitto_sub -h localhost -p 1884 -t sniffer/data -v

Når ESP32 har samlet 5 datasæt, vil den sende et JSON-array til topic sniffer/data.\
PlatformIO Libraries brugt

Disse libraries installeres automatisk via platformio.ini:

lib_deps =\
  knolleary/PubSubClient\
  adafruit/DHT sensor library@^1.4.4\
  adafruit/Adafruit Unified Sensor@^1.1.4\
  h2zero/NimBLE-Arduino@^1.2.0

    Husk også at sætte

    board_build.filesystem = littlefs

    for at aktivere LittleFS til offline-lagring.

Hardware Krav

    ESP32 DevKit (Wroom32)

    DHT11-sensor (3 eller 4 pin)

    2 LED'er (grøn og rød)

    2x 220 Ω modstande (valgfrit)

    Jumper wires + breadboard

Funktionalitet

    WiFi-sniffer i promiscuous-mode (2.4 GHz) + Bluetooth-scanning (NimBLE)

    Temp/fugt via DHT-sensor

    Offline-lagring i LittleFS, hvis WiFi/MQTT fejler

    Kanalskift hvert par sekunder (standard 3s)

    Bufferer 5 målinger i RAM

    Skifter til WiFi → sender både offline-data (fra /cache.txt) og nye data

    Hopper tilbage til promiscuous-sniffing

    Blinker LED'er for feedback

Offline-lagring med LittleFS

    Hvis ESP32 ikke kan forbinde til WiFi eller MQTT, gemmes data lokalt i en fil:

storeOffline(payload);

i /cache.txt.

Næste gang netforbindelsen er oppe, kaldes

    sendOfflineData();

    der sender alt indhold af /cache.txt og sletter filen bagefter.

Dette opfylder kravene om, at data stadig indsamles selv uden net, og senere "synkroniseres" til serveren.\
Visualisering (valgfri)\
Node-RED

    Node-RED + node-red-dashboard giver en hurtig måde at lave grafer.

    Flow:

    [MQTT in] -> [json] -> [function(for chart)] -> [ui_chart]

    Tilmeld topic sniffer/data og vis fx wifiCount, btCount, temp, humidity over tid.

Andre løsninger

    Grafana via InfluxDB + MQTT-løsning

    Egen web-frontend (Chart.js etc.)

Kørsel og Test

    Åbn projektet i VS Code (PlatformIO)

    Ret config.h (WiFi + MQTT IP etc.)

    Upload til ESP32

    Serial Monitor (Ctrl+Alt+M): se "Skifter kanal: X" og sample-linjer

    Start mosquitto:

mosquitto -c ./mosquitto.conf -v

Abonnér:

    mosquitto_sub -h localhost -p 1884 -t sniffer/data -v

    Se data, fx:

[{"timestamp":12345,"temp":24.3,"humidity":55.2,"wifiCount":3,"btCount":12}, ...]

Udvidelser (idéer)

    Filtrering på RSSI: ignorér meget svage signaler

    3 sensorer (fx CO₂, bevægelses-IR) for at få mere robust persontælling

    Databehandling (f.eks. glidende gennemsnit)

    Integration til Node-RED med dashboards

    Fler-trådet scanning eller "bedre" WiFi-scanning med data-frames vs mgmt-frames

Formål

Dette projekt demonstrerer en fuld IoT-løsning for passiv persontælling og indeklima, med:

    WiFi og BLE scanning

    DHT for temperatur/fugt

    Offline-lagring med LittleFS

    MQTT i stedet for HTTP

    Modulær kode der kan udbygges med flere sensorer

Husk: Hvis rummet kører mest på 5 GHz, ser du få/ingen WiFi-enheder -- men BT-tal vil ofte være højt.

God fornøjelse!\
--- Martin's IoT-projekt# IoT Persontælling & Indeklima -- ESP32 Projekt

Dette projekt bruger en **ESP32** til at:

- **Tælle enheder** i lokalet via **WiFi-sniffing** (2.4 GHz) og **Bluetooth** (BLE)\
- **Måle temperatur og luftfugtighed** via en DHT-sensor (eks. DHT11)\
- **Buffer og offline-lagre data** (LittleFS), hvis netforbindelse mangler\
- **Sende data** som JSON via **MQTT** efter et antal målinger

Derudover vises status med LED'er:

- ✅ Grøn LED blinker ved succesfuld måling\
- ❌ Rød LED blinker ved sensorfejl

> **Vigtig note:** ESP32 kan kun scanne **2.4 GHz** -- hvis rummet primært bruger 5 GHz WiFi, vil `wifiCount` ofte være 0.

---

## ESP32 Opsætning

Projektet er udviklet med [PlatformIO](https://platformio.org/) og kræver følgende:

### 1. Installer PlatformIO

- Installer som plugin til [Visual Studio Code](https://code.visualstudio.com/)\
- Åbn projektmappen i VS Code

### 2. Tilpas `config.h`

Find og redigér `src/config.h`:

```cpp\
#define WIFI_SSID "DIT_WIFI_NAVN"\
#define WIFI_PASS "DIN_WIFI_KODE"

#define MQTT_SERVER "DIN_PC_IP"\
#define MQTT_PORT 1884\
#define MQTT_CLIENT_ID "esp32_sniffer"\
#define MQTT_TOPIC "sniffer/data"

// Andre parametre som SAMPLE_INTERVAL_MS, BT_SCAN_DURATION_SEC, BUFFER_LIMIT, etc.

Tip: Find din PC's IP ved at skrive ipconfig i en terminal.\
Lokal MQTT Broker (Mosquitto)

Projektet bruger Mosquitto som MQTT-broker. En færdig konfigurationsfil (mosquitto.conf) er inkluderet.\
Trin 1: Installer Mosquitto

Download og installer fra:\
https://mosquitto.org/download/

Sørg for at tilføje Mosquitto til systemets PATH under installationen (eller gør det manuelt bagefter).\
Trin 2: Start broker med konfig

I projektmappen (hvor mosquitto.conf ligger), kør:

mosquitto -c ./mosquitto.conf -v

Dette starter en broker på port 1884 og tillader anonyme forbindelser.\
Trin 3: Subscribe til ESP32-data

mosquitto_sub -h localhost -p 1884 -t sniffer/data -v

Når ESP32 har samlet 5 datasæt, vil den sende et JSON-array til topic sniffer/data.\
PlatformIO Libraries brugt

Disse libraries installeres automatisk via platformio.ini:

lib_deps =\
  knolleary/PubSubClient\
  adafruit/DHT sensor library@^1.4.4\
  adafruit/Adafruit Unified Sensor@^1.1.4\
  h2zero/NimBLE-Arduino@^1.2.0

    Husk også at sætte

    board_build.filesystem = littlefs

    for at aktivere LittleFS til offline-lagring.

Hardware Krav

    ESP32 DevKit (Wroom32)

    DHT11-sensor (3 eller 4 pin)

    2 LED'er (grøn og rød)

    2x 220 Ω modstande (valgfrit)

    Jumper wires + breadboard

Funktionalitet

    WiFi-sniffer i promiscuous-mode (2.4 GHz) + Bluetooth-scanning (NimBLE)

    Temp/fugt via DHT-sensor

    Offline-lagring i LittleFS, hvis WiFi/MQTT fejler

    Kanalskift hvert par sekunder (standard 3s)

    Bufferer 5 målinger i RAM

    Skifter til WiFi → sender både offline-data (fra /cache.txt) og nye data

    Hopper tilbage til promiscuous-sniffing

    Blinker LED'er for feedback

Offline-lagring med LittleFS

    Hvis ESP32 ikke kan forbinde til WiFi eller MQTT, gemmes data lokalt i en fil:

storeOffline(payload);

i /cache.txt.

Næste gang netforbindelsen er oppe, kaldes

    sendOfflineData();

    der sender alt indhold af /cache.txt og sletter filen bagefter.

Dette opfylder kravene om, at data stadig indsamles selv uden net, og senere "synkroniseres" til serveren.\
Visualisering (valgfri)\
Node-RED

    Node-RED + node-red-dashboard giver en hurtig måde at lave grafer.

    Flow:

    [MQTT in] -> [json] -> [function(for chart)] -> [ui_chart]

    Tilmeld topic sniffer/data og vis fx wifiCount, btCount, temp, humidity over tid.

Andre løsninger

    Grafana via InfluxDB + MQTT-løsning

    Egen web-frontend (Chart.js etc.)

Kørsel og Test

    Åbn projektet i VS Code (PlatformIO)
    Ret config.h (WiFi + MQTT IP etc.)
    Upload til ESP32
    Serial Monitor (Ctrl+Alt+M): se "Skifter kanal: X" og sample-linjer
    Start mosquitto:

mosquitto -c ./mosquitto.conf -v

Abonnér:

    mosquitto_sub -h localhost -p 1884 -t sniffer/data -v
    Se data, fx:

[{"timestamp":12345,"temp":24.3,"humidity":55.2,"wifiCount":3,"btCount":12}, ...]

Udvidelser (idéer)

    Filtrering på RSSI: ignorér meget svage signaler
    3 sensorer (fx CO₂, bevægelses-IR) for at få mere robust persontælling
    Databehandling (f.eks. glidende gennemsnit)
    Integration til Node-RED med dashboards
    Fler-trådet scanning eller "bedre" WiFi-scanning med data-frames vs mgmt-frames

Formål

Dette projekt demonstrerer en fuld IoT-løsning for passiv persontælling og indeklima, med:

    WiFi og BLE scanning
    DHT for temperatur/fugt
    Offline-lagring med LittleFS
    MQTT i stedet for HTTP
    Modulær kode der kan udbygges med flere sensorer

Husk: Hvis rummet kører mest på 5 GHz, ser du få/ingen WiFi-enheder -- men BT-tal vil ofte være højt.

God fornøjelse!\
--- Martin's IoT-projekt