IoT Persontælling & Indeklima – ESP32 Projekt
=============================================

Dette projekt bruger en **ESP32** til at måle:

*   Antal enheder i nærheden (via WiFi- og Bluetooth-scanning)
*   Temperatur og luftfugtighed (via DHT11)
*   Data sendes samlet som JSON via MQTT efter et antal målinger

Derudover vises status med LED'er:

*   ✅ Grøn LED blinker ved succesfuld måling
*   ❌ Rød LED blinker ved sensorfejl

ESP32 Opsætning
---------------

Projektet er udviklet med [PlatformIO](https://platformio.org/) og kræver følgende:

### 1\. Installer PlatformIO

*   Installer som plugin til [Visual Studio Code](https://code.visualstudio.com/)
*   Åbn projektmappen i VS Code

### 2\. Tilpas config.h

Find og redigér src/config.h:

```c
#define WIFI_SSID "DIT_WIFI_NAVN"  
#define WIFI_PASS "DIN_WIFI_KODE"  
#define MQTT_SERVER "DIN_PC_IP"  
#define MQTT_PORT 1884
```

_Find din PC’s IP ved at skrive ipconfig i en terminal._

MQTT Broker – Lokal Opsætning
-----------------------------

Projektet bruger **Mosquitto** som MQTT-broker. En færdig konfigurationsfil (mosquitto.conf) er inkluderet.

### Trin 1: Installer Mosquitto

Download og installer fra:https://mosquitto.org/download/

Sørg for at tilføje Mosquitto til **systemets PATH** under installationen (eller gør det manuelt bagefter).

### Trin 2: Start broker med konfig

Åbn en terminal i projektmappen og kør:

```bash
mosquitto -c ./mosquitto.conf -v
```

Dette starter en broker på **port 1884** og tillader anonyme forbindelser.

### Trin 3: Subscribe til ESP32-data

I en ny terminal, kør:

```bash
mosquitto_sub -h localhost -p 1884 -t sniffer/data -v 
```

Når ESP32 har samlet 5 datasæt, vil den sende en JSON-array til topic sniffer/data.

PlatformIO Libraries brugt
--------------------------

Disse libraries installeres automatisk via platformio.ini:

lib_deps =    
    knolleary/PubSubClient    
    adafruit/DHT sensor library@^1.4.4    
    adafruit/Adafruit Unified Sensor@^1.1.4    
    h2zero/NimBLE-Arduino@^1.2.0

Hardware Krav
-------------

*   ESP32 DevKit (Wroom32)
*   DHT11 sensor (3 eller 4 pin)
*   2x LED (grøn og rød)
*   2x 220Ω modstande (valgfrit)
*   Jumper wires + breadboard

Funktionalitet
--------------

*   Samler WiFi/Bluetooth enheder (MAC, RSSI)
*   Læser temperatur og fugt
*   Bufferer 5 målinger i ESP32 RAM
*   Skifter til WiFi → sender samlet JSON til MQTT
*   Hopper af WiFi og fortsætter sniffing
*   Blinker LED’er som feedback

Visualisering (valgfri)
-----------------------

Data kan vises i:

*   **Node-RED** (MQTT in → JSON → Chart)
*   **Grafana** via InfluxDB + MQTT bridge
*   **Custom frontend** (Chart.js, React, Vue, m.m.)

Test
----

Kør mosquitto\_sub, vent på at ESP32 sender data, og se output i terminalen:

```bash
mosquitto_sub -h localhost -p 1884 -t sniffer/data -v 
```

Udvidelser (idéer)
------------------

*   Automatisk fallback til SPIFFS hvis WiFi er nede
*   Flere sensorer (CO₂, bevægelse)
*   Web-dashboard med live MQTT-graf
*   Data-analyse baseret på RSSI og temperatur 

Formål
------

Dette projekt er lavet til uddannelsesformål og demonstrerer en realistisk brug af IoT til **passiv persontælling og miljømåling** i et lokale – med mulighed for at udvide til mange formål.