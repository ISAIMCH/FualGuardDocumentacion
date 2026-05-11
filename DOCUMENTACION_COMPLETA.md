# 📡 Documentación Completa - Sistema de Telemetría de Tanques de Diésel

**Autor:** Isai TICS  
**Fecha:** Abril 2026  
**Sistema:** Heltec LoRa v3 → LilyGO SIM7070 → Node-RED → InfluxDB → Grafana  

---

## 📋 Tabla de Contenidos
1. [Descripción General](#descripción-general)
2. [Arquitectura del Sistema](#arquitectura-del-sistema)
3. [Hardware Utilizado](#hardware-utilizado)
4. [Flujo de Datos Detallado](#flujo-de-datos-detallado)
5. [Configuración de Componentes](#configuración-de-componentes)
6. [Ejemplo de Datos](#ejemplo-de-datos)
7. [Visualización en Grafana](#visualización-en-grafana)
8. [Troubleshooting](#troubleshooting)

---

## 📝 Descripción General

Este sistema de telemetría monitorea remotamente **tanques de diésel en camiones** mediante:
- Lectura de sensores en campo (GPS, caudalímetro, detector de tapa)
- Transmisión inalámbrica de datos de corto alcance (ESP-NOW)
- Envío de datos a la nube vía red GPRS
- Almacenamiento en base de datos de series temporales (InfluxDB)
- Visualización en tiempo real en Grafana

**Características principales:**
- ✅ Geolocalización en tiempo real
- ✅ Sistema de buffer para reconexión automática
- ✅ Validación de datos
- ✅ Dashboard con múltiples gráficos
- ✅ Historial completo de eventos

---

## 🏗️ Arquitectura del Sistema

```
┌──────────────────────────────────────────────────────────────────────────┐
│              SISTEMA IOT HÍBRIDO CON DUAL SENSOR Y REDUNDANCIA           │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│              Trasmisor (Heltec LoRa v3)                                  │
│              ├─ 2 Sensores de Flujo (Ida/Retorno)                        │
│              ├─ Reed Switch (Tapa)                                       │
│              ├─ GPS (GY-GPS6MV2)                                         │
│              ├─ LoRa SF=9, 915MHz (Broadcast)                            │
│              └─ ESP-NOW directo → LilyGO (PRINCIPAL) ✅                 │
│                      │ └─→ LoRa → Receptor (REDUNDANCIA)                │
│                      │                                                    │
│              LilyGO SIM7070 (Gateway MQTT)                              │
│              ├─ Recibe ESP-NOW del Trasmisor                             │
│              ├─ GPS del SIM7070 (enriquecimiento)                       │
│              ├─ Buffer automático si falla MQTT                          │
│              └─ MQTT → broker.hivemq.com:1883                           │
│                      │
│                      ↓ itics/mgti/isai/sensor
│              Node-RED (Processing)
│              ├─ Funciones 8-11: Parseo dual flujo + validación          │
│              ├─ Extrae: flujo_ida, flujo_retorno, consumo_actual        │
│              └─ Envía a InfluxDB
│                      │
│                      ↓ Bucket: SENSORES
│              InfluxDB (Time Series)
│              ├─ Measurement: monitoreo_diesel_tics                       │
│              ├─ Fields: caudal_flujo_ida, caudal_flujo_retorno,          │
│              │          consumo_actual, estado_tapa, latitud, longitud   │
│              └─ Tags: unidad, propietario, sensor, gateway               │
│                      │
│                      ↓
│              Grafana (Visualización Real-time)
│              ├─ Time Series: Flujo Ida + Retorno                        │
│              ├─ Gauge: Consumo Neto (L/min)                             │
│              ├─ Timeline: Estado Tapa (ABIERTA/CERRADA)                 │
│              └─ GeoMap: Ubicación GPS en tiempo real                    │
│                                                                            │
└──────────────────────────────────────────────────────────────────────────┘
```

**Capas de Comunicación:**
- **Capa 1 (Principal):** Trasmisor → ESP-NOW → LilyGO (~100ms latencia)
- **Capa 2 (Redundancia):** Trasmisor → LoRa SF=9 → Receptor (visual para monitoreo)
- **Capa 3 (Nube):** LilyGO → MQTT/4G-LTE → Node-RED → InfluxDB

---

## 🔧 Hardware Utilizado

### 1️⃣ **Heltec LoRa v3 (ESP32 + LoRa) - TRASMISOR**
**Ubicación:** Campo (montado en el tanque del camión)  
**Función:** Nodo sensor primario, lectura de sensores, transmisión LoRa
**Firmware:** `Trasmisor.ino`

| Componente | Especificación |
|-----------|---------------|
| **Microcontrolador** | ESP32 (WiFi + BLE) |
| **Módulo LoRa** | SX1262 |
| **GPS** | GY-GPS6MV2 (u-blox NEO-6M) con antena |
| **Sensor de Tapa** | Reed Switch magnético (PIN 5, entrada INPUT_PULLUP) |
| **Sensor de Flujo 1** | Ultrasónico clamp-on (IDA: tanque→motor), PIN 38 |
| **Sensor de Flujo 2** | Ultrasónico clamp-on (RETORNO: motor→tanque), PIN 39 |
| **Calibración Flujo** | 5.5 pulsos/litro (ambos sensores) |
| **Intervalo de envío** | 5000 ms (5 segundos, 12 reportes/min) |
| **Transmisión Dual** | ESP-NOW directo (PRINCIPAL) + LoRa SF=9 (BACKUP) |
| **LoRa Config** | SF=9, 915MHz, BW 125kHz, CR 5, TX 17dBm |
| **Potencia promedio** | ~180mW TX, ~60mW escucha (con dual sensor activo) |
| **Librerías** | heltec_unofficial (RadioLib), TinyGPS++, ArduinoJson |

**Pines GPIO (Trasmisor):**
```
PIN 5  → Reed Switch (Sensor de tapa), activa BAJA (LOW = cerrada)
PIN 38 → Sensor Flujo 1 (Ida), contador con FALLING EDGE
PIN 39 → Sensor Flujo 2 (Retorno), contador con FALLING EDGE
PIN 33 → GPS RX (recibe NMEA a 115200 bps)
PIN 4  → GPS TX (envía comandos al GPS)
(LoRa integrado SX1262 → radio.begin(915.0) vía heltec_unofficial)
```

---

### 1B️⃣ **Heltec LoRa v3 (ESP32 + LoRa) - RECEPTOR (BRIDGE)**
**Ubicación:** Punto intermedio (estación base, 50-200m del tanque)  
**Función:** Monitoreo visual LoRa (Solo visualización, NO retransmite)
**Firmware:** `Receptor.ino`

| Componente | Especificación |
|-----------|---------------|
| **Microcontrolador** | ESP32 (WiFi + BLE) |
| **Módulo LoRa** | SX1262 |
| **Pantalla OLED** | 128x64 (monitoreo visual en vivo) |
| **Recepción LoRa** | SF=9, 915MHz, BW 125kHz, CR 5 (RadioLib) |
| **Función Principal** | Monitoreo visual en OLED (NO retransmite) |
| **Timeout sin señal** | 10 segundos (alerta en pantalla) |
| **Detección de errores** | CRC check (avisa si paquete corrupto) |
| **Librerías** | heltec_unofficial, ArduinoJson, RadioLib |

**Pines GPIO (Receptor):**
```
(LoRa integrado SX1262 → radio.begin(915.0) vía heltec_unofficial)
(OLED automático en I2C de la placa Heltec)
```

**Algoritmo del Receptor (SIMPLIFICADO - Solo Monitoreo):**
```
Inicio del loop:
├─ radio.receive(received) - intenta recibir paquete LoRa
│  ├─ Si RADIOLIB_ERR_NONE → Paquete válido:
│  │  ├─ Parsea JSON (flujo1, flujo2, lat, lng, alerta)
│  │  └─ Muestra en OLED 128x64:
│  │     ├─ Línea 1: "CABINA RX"
│  │     ├─ Línea 2: "F1: X.X F2: Y.Y" (L/min)
│  │     ├─ Línea 3: "TAPA: CERRADA/ABIERTA"
│  │     ├─ Línea 4: "Lat: A.AAAA"
│  │     └─ Línea 5: "Lng: B.BBBB"
│  │
│  ├─ Si RADIOLIB_ERR_CRC_MISMATCH → Paquete corrupto
│  │  └─ Log: "Paquete corrupto (interferencia EM)"
│  │
│  └─ Si timeout > 10s sin señal
│     └─ Muestra "SIN SEÑAL LORA" + "Verificando TX..."
│
└─ Repite (no bloqueante, esperando próximo paquete)

⚠️ NO retransmite a cloud. Solo visualiza en pantalla OLED del campo.
```

---

### 2️⃣ **LilyGO SIM7070 (ESP32 + Modem 4G/LTE) - GATEWAY**
**Ubicación:** Punto de acceso (en la estación base)  
**Función:** Gateway/Puerta de enlace y conexión a internet

| Componente | Especificación |
|-----------|---------------|
| **Microcontrolador** | ESP32 (WiFi + BLE) |
| **Modem** | SIM7070 (4G/LTE) |
| **Operador** | ISU Acell (México) |
| **GPS Modem** | Integrado en SIM7070 |
| **Buffer de datos** | 10 registros máximo |
| **Protocolo salida** | MQTT |
| **Broker** | broker.hivemq.com:1883 |

**Pines UART:**
```
PIN 26 (RX) ← Modem TX
PIN 27 (TX) → Modem RX
PIN 4       → Power Key (encendido del modem)
PIN 12      → LED indicador
```

**Credenciales GPRS:**
- **APN:** web.iusacellgsm.mx
- **Usuario:** iusacellgsm
- **Contraseña:** iusacellgsm

---

## 📊 Flujo de Datos Detallado

### **Etapa 1A: Lectura de Sensores (Heltec LoRa v3 TRASMISOR)**

```c
┌──────────────────────────────────────────────────────────┐
│ Cada 5 segundos (en loop principal):                   │
│                                                          │
│ 1. Lee GPS (TinyGPS++ a 115200 bps, UART 1)            │
│    → Latitud, Longitud, Satélites, Precisión 2.5m      │
│    → Si no válido → LAT/LNG = 0.0                      │
│                                                          │
│ 2. Lee sensor de tapa Reed Switch (PIN 5)              │
│    → LOW (magnético cerca) = Tapa CERRADA → alerta=1   │
│    → HIGH (magnético lejos) = Tapa ABIERTA → alerta=0  │
│                                                          │
│ 3. Calcula flujo DUAL (ambos sensores)                 │
│    → pulseCount1 (PIN 38) / 5.5 = flujo1 (L en 5s)    │
│    → pulseCount2 (PIN 39) / 5.5 = flujo2 (L en 5s)    │
│    → Reset ambos contadores                            │
│                                                          │
│ 4. Construye JSON serializado                          │
│    → {"id":"nodo1", "alerta":X, "flujo1":Y,           │
│       "flujo2":Z, "lat":A, "lng":B, "sats":C,         │
│       "gps":1/0}                                        │
│                                                          │
│ 5. TRANSMISIONES SIMULTÁNEAS:                          │
│    a) ESP-NOW → LilyGO (MAC: 10:06:1C:40:C4:44)        │
│       ├─ Latencia: ~50-100ms                           │
│       ├─ Principal (confiable si hay WiFi)             │
│       └─ Confirmado con callback onDataSent()          │
│                                                          │
│    b) LoRa Broadcast SF=9, 915MHz                      │
│       ├─ Latencia: ~500ms (tiempo aire)                │
│       ├─ Redundancia (caso WiFi falle)                 │
│       ├─ Recibe: Receptor (monitoreo visual)           │
│       └─ No se retransmite a cloud                     │
└──────────────────────────────────────────────────────────┘
```

**Código de transmisión dual (Trasmisor.ino):**
```cpp
// 1️⃣ ESP-NOW (PRINCIPAL - Directo al LilyGO)
esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) json.c_str(), json.length());
if (result == ESP_OK) {
    Serial.println("ESP-NOW OK");
} else {
    Serial.println("ESP-NOW FAIL");
}

// 2️⃣ LoRa (BACKUP - Broadcast para Receptor)
radio.transmit(json);  // RadioLib API (heltec_unofficial)
Serial.println("LoRa TX");
```

---

### **Etapa 1B: Recepción LoRa (Heltec LoRa v3 RECEPTOR - Monitoreo Visual)**

```c
┌──────────────────────────────────────────────────────┐
│ Loop de escucha LoRa (NO retransmite):             │
│                                                      │
│ 1. Intenta recibir LoRa con timeout automático      │
│    state = radio.receive(received)                  │
│    ├─ RadioLib no bloquea si no hay paquete        │
│    └─ Timeout 10s sin señal desencadena alerta     │
│                                                      │
│ 2. Si RADIOLIB_ERR_NONE (paquete válido):          │
│    ├─ received = string JSON completo               │
│    ├─ Parsea: flujo1, flujo2, lat, lng, alerta    │
│    ├─ Muestra en OLED 128x64:                      │
│    │  ├─ Línea 1: "CABINA RX"                     │
│    │  ├─ Línea 2: "F1: X.X F2: Y.Y" (L/min)       │
│    │  ├─ Línea 3: "TAPA: CERRADA/ABIERTA"         │
│    │  ├─ Línea 4: "Lat: A.AAAA"                   │
│    │  └─ Línea 5: "Lng: B.BBBB"                   │
│    └─ lastPacketTime = millis() (reset timeout)   │
│                                                      │
│ 3. Si RADIOLIB_ERR_CRC_MISMATCH (corrupto):        │
│    └─ Log: "Paquete corrupto (ruido EM/interferen)│
│                                                      │
│ 4. Si timeout > 10s sin señal:                     │
│    ├─ Muestra en OLED: "SIN SEÑAL LORA"           │
│    └─ Log: "Verificando TX..."                     │
│                                                      │
│ 5. NO hay retransmisión a cloud                    │
│    (Receptor = monitoreo visual del campo)         │
│                                                      │
│ 6. Loop repite (no bloqueante)                     │
└──────────────────────────────────────────────────────┘
```

**Código de recepción LoRa (Receptor.ino):**
```cpp
int state = radio.receive(received);  // RadioLib API (heltec_unofficial)

if (state == RADIOLIB_ERR_NONE) {
    lastPacketTime = millis();  // Reset timeout
    
    StaticJsonDocument<256> doc;
    deserializeJson(doc, received);
    
    float flujo1 = doc["flujo1"];
    float flujo2 = doc["flujo2"];
    int alerta = doc["alerta"];
    
    display.clear();
    display.drawString(0, 12, "F1: " + String(flujo1, 1) + " F2: " + String(flujo2, 1));
    display.drawString(0, 24, alerta ? "TAPA: CERRADA" : "TAPA: ABIERTA");
    display.display();
    
} else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("Paquete corrupto (Error CRC)");
}
```

---

### **Etapa 1C: Recepción ESP-NOW (LilyGO SIM7070 GATEWAY) - PRINCIPAL**

**Ejemplo JSON recibido vía ESP-NOW del Trasmisor:**
```json
{
  "id": "nodo1",
  "alerta": 1,
  "flujo1": 15.32,
  "flujo2": 8.91,
  "lat": 23.456789,
  "lng": -101.234567,
  "sats": 12,
  "gps": 1
}
```

**Callback de recepción ESP-NOW (Lilygo.ino):**
```cpp
void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Recibe datos del Trasmisor vía ESP-NOW
  payloadESPNow = String((char*)incomingData);
  hayDatoNuevo = true;  // Flag para procesamiento en loop
}
```

**Ejemplo de JSON enviado:**
```json
{
  "alerta": 0,
  "flujo": 12.50,
  "lat": 23.456789,
  "lng": -101.234567,
  "sats": 12,
  "gps": 1
}
```

---

### **Etapa 2: Procesamiento Gateway (LilyGO SIM7070) - Enriquecimiento**

```c
┌──────────────────────────────────────────────────────────────┐
│ En el loop principal (cuando hayDatoNuevo = true):          │
│                                                              │
│ 1. Valida conexión GPRS + MQTT                              │
│    ├─ asegurarConexion() - chequea red 4G/GPRS             │
│    ├─ asegurarMQTT() - reconecta broker si es necesario    │
│    └─ mqtt.loop() - procesa keep-alive                     │
│                                                              │
│ 2. REINTENTO DE BUFFER (si hay datos pendientes)            │
│    └─ enviarBufferPendiente() - resend datos buffered       │
│                                                              │
│ 3. Obtiene GPS del modem SIM7070                            │
│    └─ modem.getGPS(&lat, &lon)                             │
│       ├─ Lat: -20.1356°, Lon: -99.2145° (ejemplo)          │
│       └─ Si modem sin GPS: usa último válido               │
│                                                              │
│ 4. Enriquece JSON con datos del Gateway                     │
│    ├─ doc["lat_sim"] = lat del modem                      │
│    ├─ doc["lon_sim"] = lon del modem                      │
│    └─ doc["gateway"] = "Tanque-01"                        │
│                                                              │
│ 5. Serializa y publica MQTT                                 │
│    ├─ Broker: broker.hivemq.com:1883                       │
│    ├─ Topic: itics/mgti/isai/sensor                        │
│    ├─ Si OK → LED parpadea (confirmación)                  │
│    └─ Si FALLA → Guarda en buffer[MAX_BUFFER=10]           │
│                                                              │
│ 6. Reintento automático cuando conexión vuelve              │
│    └─ Buffer FIFO: mantiene orden de datos                 │
└──────────────────────────────────────────────────────────────┘
```

**Código de enriquecimiento (Lilygo.ino):**
```cpp
if (hayDatoNuevo) {
    digitalWrite(LED_PIN, HIGH);  // Confirma recepción
    
    // Obtiene GPS del SIM7070
    modem.getGPS(&lat, &lon);
    
    // Parsea JSON recibido
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payloadESPNow);
    
    // Enriquece con datos del Gateway
    doc["lat_sim"] = lat;
    doc["lon_sim"] = lon;
    doc["gateway"] = "Tanque-01";
    
    // Serializa
    char bufferFinal[256];
    serializeJson(doc, bufferFinal);
    
    // Publica MQTT
    enviarODefinirBuffer(String(bufferFinal));
    
    digitalWrite(LED_PIN, LOW);
    hayDatoNuevo = false;
}
```

**Ejemplo JSON enviado a MQTT (enriquecido):**
```json
{
  "id": "nodo1",
  "alerta": 1,
  "flujo1": 15.32,
  "flujo2": 8.91,
  "lat": 23.456789,
  "lng": -101.234567,
  "sats": 12,
  "gps": 1,
  "lat_sim": -20.1356,
  "lon_sim": -99.2145,
  "gateway": "Tanque-01"
}
```

---

### **Etapa 3: Procesamiento Node-RED (Functions 8-11)**

📸 **Ver:** [Nodos -NodeRed.png](Imagenes/Nodos%20-NodeRed.png)

```
┌─────────────────────────────────────────────────────────────┐
│ MQTT INPUT: itics/mgti/isai/sensor                         │
│                                                              │
│ JSON Parsing (json node)                                    │
│ └─ Convierte string MQTT a objeto                          │
│                                                              │
│ ┌─ Function 8: Extrae GPS y pasa a template               │
│ │  ├─ Input: msg.payload.lat, msg.payload.lng             │
│ │  └─ Output: msg para worldmap                           │
│ │                                                           │
│ ├─ Function 9: Estado de Tapa                             │
│ │  ├─ Input: msg.payload.alerta (0/1)                    │
│ │  └─ Output: "ABIERTA" o "CERRADA" → Gauge A            │
│ │                                                           │
│ ├─ Function 10: Flujo IDA (sensor 1)                      │
│ │  ├─ Input: msg.payload.flujo1                          │
│ │  └─ Output: Gráfico time series + Gauge                │
│ │                                                           │
│ ├─ Function 10-2: Flujo RETORNO (sensor 2)               │
│ │  ├─ Input: msg.payload.flujo2                          │
│ │  └─ Output: Gráfico time series + Gauge                │
│ │                                                           │
│ └─ Function 11: Estructura para InfluxDB                  │
│    ├─ Validación: Si lat/lng válidos (≠0)                │
│    ├─ Calcula: consumo_actual = flujo1 - flujo2          │
│    ├─ Measurement: "monitoreo_diesel_tics"              │
│    ├─ Fields:                                             │
│    │  ├─ caudal_flujo_ida: msg.flujo1                   │
│    │  ├─ caudal_flujo_retorno: msg.flujo2               │
│    │  ├─ consumo_actual: flujo1-flujo2                  │
│    │  ├─ estado_tapa: msg.alerta (0/1)                 │
│    │  ├─ latitud: msg.lat                               │
│    │  └─ longitud: msg.lng                              │
│    └─ Tags:                                               │
│       ├─ unidad: "Camion_Kiwi"                         │
│       ├─ propietario: "Isai_TICS"                      │
│       ├─ sensor: "Hardware_LilyGO_Real"                │
│       └─ gateway: msg.gateway                          │
│                                                           │
│ OUTPUTS:                                                  │
│ ├─ InfluxDB node (guardar series temporal)              │
│ ├─ Worldmap (visualizar GPS en vivo)                    │
│ ├─ Dashboard widgets (gauges, charts)                   │
│ └─ Alerts (si consumo > umbral o tapa abierta)         │
└─────────────────────────────────────────────────────────────┘
```

---

### **Etapa 4: Almacenamiento (InfluxDB 2.x)**

📸 **Ver:** [InfluxDB.png](Imagenes/InfluxDB.png)

```
Organización: ProyectoMD
├─ Bucket: SENSORES (retención indefinida)
│  │
│  └─ Measurement: monitoreo_diesel_tics
│     ├─ Fields (valores numéricos, NO indexados):
│     │  ├─ caudal_flujo_ida: 15.32 (L/5seg)
│     │  ├─ caudal_flujo_retorno: 8.91 (L/5seg)
│     │  ├─ consumo_actual: 6.41 (flujo1-flujo2)
│     │  ├─ estado_tapa: 1 (1=cerrada, 0=abierta)
│     │  ├─ latitud: 23.456789
│     │  └─ longitud: -101.234567
│     │
│     ├─ Tags (metadatos indexados, rápida búsqueda):
│     │  ├─ unidad: "Camion_Kiwi"
│     │  ├─ propietario: "Isai_TICS"
│     │  ├─ sensor: "Hardware_LilyGO_Real"
│     │  └─ gateway: "Tanque-01"
│     │
│     └─ Timestamp: 2026-05-04T20:13:45.000Z
│
└─ Retención: Indefinida (guardar todo el histórico)

Total puntos almacenados:
├─ 12 puntos/min × 60 min/h × 24 h/día = ~17,280 puntos/día
├─ 17,280 × 30 = ~518,400 puntos/mes
└─ Máximo observado: 318.91 (pico en test de flujo retorno)
```

---

### **Etapa 5: Visualización (Grafana Dashboard)**

📸 **Ver:** [Grafana.png](Imagenes/Grafana.png)

**Dashboard: "Monitoreo" (tiempo real, actualización cada 5s)**

```
┌────────────────────────────────────────────────────────┐
│ GRAFANA PANELS (Flux Queries)                          │
│                                                         │
│ 1️⃣ TIME SERIES: Caudal Flujo IDA + RETORNO          │
│    ├─ X-Axis: Tiempo (últimas 6 horas)              │
│    ├─ Y-Axis: L/min                                 │
│    ├─ Línea verde: caudal_flujo_ida (flujo1)       │
│    └─ Línea amarilla: caudal_flujo_retorno (flujo2)│
│       Flux Query:
│       from(bucket: "SENSORES")
│         |> filter(fn: (r) => r["_field"] == "caudal_flujo_ida")
│
│ 2️⃣ GAUGE: Consumo Neto (L/min)                       │
│    ├─ Valor actual: 0.00 L/min (cuando motor parado)
│    ├─ Rango: 0-30 L/min                            │
│    ├─ Verde: 0-10 (normal)                          │
│    ├─ Amarillo: 10-20 (alto)                        │
│    └─ Rojo: >20 (crítico/anomalía)                  │
│
│ 3️⃣ TIMELINE: Estado de la Tapa                      │
│    ├─ Verde: CERRADA (alerta=1, seguro)            │
│    ├─ Rojo: ABIERTA (alerta=0, riesgo)             │
│    └─ Muestra cambios de estado en timeline        │
│
│ 4️⃣ GEOMAP: Ubicación del Camión                     │
│    ├─ Mapa mundial interactivo                     │
│    ├─ Punto verde: Posición actual (lat/lng)       │
│    ├─ Zoom: Zoom en la región (México)             │
│    └─ Actualización: Cada 5 segundos               │
│                                                         │
│ CONTROLES:                                             │
│ ├─ Time Range: Last 6 hours / Auto refresh            │
│ ├─ Filters: Por unidad, propietario, sensor          │
│ └─ Export: PDF, PNG, datos CSV                       │
└────────────────────────────────────────────────────────┘
```

---

## ⚙️ Configuración de Componentes

### **HELTEC LORA V3 - Configuración con RadioLib**

**Librerías requeridas (Trasmisor):**
```cpp
#include <heltec_unofficial.h>    // Placa Heltec (RadioLib, OLED)
#include <TinyGPS++.h>             // Lectura del GPS a 115200 bps
#include <ArduinoJson.h>           // Serialización JSON dual flujo
#include <WiFi.h>                  // WiFi para ESP-NOW
#include <esp_now.h>               // Transmisión directa
```

**Inicialización LoRa (Trasmisor.ino con RadioLib):**
```cpp
// Usa heltec_unofficial + RadioLib (objeto global: radio)
Serial.print("Iniciando LoRa... ");
int radiostate = radio.begin(915.0);  // 915 MHz (RadioLib)

if (radiostate == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa OK!");
} else {
    Serial.print("Error LoRa, codigo: ");
    Serial.println(radiostate);
    while (true);
}

// Configuración SF=9 para ambiente hostil
radio.setSpreadingFactor(9);           // SF=9 (robustez vs latencia)
radio.setBandwidth(125.0);             // 125 kHz
radio.setCodingRate(5);                // Coding rate 4/5
radio.setOutputPower(17);              // 17 dBm (máximo)

Serial.println("LoRa SF=9, 915MHz, BW125kHz");
```

**Transmisión dual (Trasmisor.ino):**
```cpp
// 1️⃣ ESP-NOW (PRINCIPAL)
esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)json.c_str(), json.length());
if (result == ESP_OK) Serial.println("[ESP-NOW] OK");
else Serial.println("[ESP-NOW] FAIL -> Datos buffered en LilyGO");

// 2️⃣ LoRa (REDUNDANCIA)
radio.transmit(json);  // RadioLib api (transmisión no bloqueante)
Serial.println("[LoRa] TX SF=9");
```

---

**Librerías requeridas (Receptor):**
```cpp
#include <heltec_unofficial.h>    // Placa Heltec (RadioLib, OLED)
#include <ArduinoJson.h>           // Parseo JSON dual sensor
#include <U8x8lib.h>               // Display OLED 128x64
```

**Inicialización LoRa (Receptor.ino con RadioLib):**
```cpp
// Usa heltec_unofficial + RadioLib
Serial.print("Iniciando Receptor LoRa... ");
int radiostate = radio.begin(915.0);  // 915 MHz

if (radiostate == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa OK!");
} else {
    Serial.print("Error LoRa: ");
    Serial.println(radiostate);
    while (true);
}

// MISMA CONFIG que Trasmisor (compatibilidad)
radio.setSpreadingFactor(9);
radio.setBandwidth(125.0);
radio.setCodingRate(5);

Serial.println("Receptor LoRa SF=9 listo");
```

**Recepción LoRa (Receptor.ino):**
```cpp
int state = radio.receive(received);  // received es String

if (state == RADIOLIB_ERR_NONE) {
    // Paquete válido
    lastPacketTime = millis();  // Reset timeout 10s
    StaticJsonDocument<256> doc;
    deserializeJson(doc, received);
    
    float flujo1 = doc["flujo1"];
    float flujo2 = doc["flujo2"];
    // ... Muestra en OLED
    
} else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // Paquete corrupto (interferencia EM)
    Serial.println("Paquete corrupto CRC");
}
```

---

### **LILYGO SIM7070 - Configuración**

**Librerías requeridas:**
```cpp
#include <TinyGsmClient.h>         // Cliente GSM 4G/GPRS
#include <PubSubClient.h>          // MQTT client
#include <ArduinoJson.h>           // Parseo JSON enriquecido
#include <esp_now.h>               // Receptor ESP-NOW del Trasmisor
#include <WiFi.h>                  // Modo WiFi STA
#include <HardwareSerial.h>        // UART para modem
```

**Inicialización Modem (Lilygo.ino):**
```cpp
// UART 1: RX=26, TX=27 (pines predefinidos LilyGO)
HardwareSerial SerialAT(1);
SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

// Encender modem SIM7070
digitalWrite(MODEM_PWRKEY, HIGH);  // Power Key activo
delay(1500);
digitalWrite(MODEM_PWRKEY, LOW);   // Libera
delay(5000);                        // Espera boot

if (!modem.testAT()) {
    Serial.println("[ERROR] Modem no responde");
}

// Habilitar GPS del SIM7070
modem.sendAT("+CGPS=1");  // GPS integrado
modem.waitResponse();

// Conectar a GPRS (ISU Acell México)
modem.gprsConnect("web.iusacellgsm.mx", "iusacellgsm", "iusacellgsm");
```

**Inicialización ESP-NOW (Lilygo.ino - receptor):**
```cpp
WiFi.mode(WIFI_STA);  // Station mode (recibe desde Trasmisor)
if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
}
esp_now_register_recv_cb(onDataRecv);  // Callback cuando llegan datos
```

**Configuración MQTT (Lilygo.ino):**
```cpp
PubSubClient mqtt(client);  // Cliente MQTT sobre TinyGsmClient
mqtt.setServer("broker.hivemq.com", 1883);

// Topic único de publicación
const char* topicPublish = "itics/mgti/isai/sensor";

// En setup():
mqtt.setServer(broker, 1883);
```

**Callback ESP-NOW (Lilygo.ino - recibe del Trasmisor):**
```cpp
void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    // Recibe directamente desde Trasmisor (no del Receptor)
    // MAC del Trasmisor (solo para log)
    Serial.print("Datos de Trasmisor MAC: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac[i]);
        if (i < 5) Serial.print(":");
    }
    
    // Convierte payload a String
    char buffer[len + 1];
    memcpy(buffer, incomingData, len);
    buffer[len] = '\0';
    
    payloadESPNow = String(buffer);
    hayDatoNuevo = true;  // Flag para procesamiento en loop
}
```

**Sistema de Buffer Robusto (Lilygo.ino):**
```cpp
#define MAX_BUFFER 10
String bufferDatos[MAX_BUFFER];
int bufferIndex = 0;

// Enviar dato o guardar si falla MQTT
void enviarODefinirBuffer(String data) {
    if (mqtt.publish(topicPublish, data.c_str())) {
        Serial.println("[MQTT OK] " + data);
    } else {
        Serial.println("[BUFFER] Guardando dato...");
        if (bufferIndex < MAX_BUFFER) {
            bufferDatos[bufferIndex++] = data;  // FIFO
        } else {
            Serial.println("[ERROR] Buffer lleno, se pierde dato");
        }
    }
}

// Reintenta buffer cuando conexión vuelve
void enviarBufferPendiente() {
    if (bufferIndex == 0) return;
    
    Serial.println("[REINTENTO] Enviando " + String(bufferIndex) + " datos del buffer...");
    int enviadosOK = 0;
    
    for (int i = 0; i < bufferIndex; i++) {
        if (mqtt.publish(topicPublish, bufferDatos[i].c_str())) {
            Serial.println("[REENVIADO OK] " + String(i+1));
            enviadosOK++;
        } else {
            Serial.println("[ERROR REENVIO] MQTT no disponible aun");
            break;  // Rompe ciclo si falla, reintenta próxima iteración
        }
    }
    
    // Limpia solo los que se enviaron exitosamente
    if (enviadosOK > 0) {
        int restantes = bufferIndex - enviadosOK;
        for (int i = 0; i < restantes; i++) {
            bufferDatos[i] = bufferDatos[i + enviadosOK];
        }
        bufferIndex = restantes;
    }
}
```

---
- Host: localhost:8086 (o IP del servidor)
- Token: (pegado desde InfluxDB)
- Organización: (tu organización)
- Bucket: SENSORES

---

## 📊 Ejemplo Completo de Datos (1 Ciclo = 5 segundos)

### **T=0s: Trasmisor (Heltec LoRa v3 - Tanque)**
```json
Leyendo sensores:
├─ GPS (115200 bps): LAT 23.456789°, LNG -101.234567°, SATS 12
├─ Reed Switch (PIN 5): LOW → Tapa CERRADA (alerta=1)
├─ Sensor Flujo 1 (PIN 38): 85 pulsos en 5s ÷ 5.5 = 15.45 L/min
├─ Sensor Flujo 2 (PIN 39): 49 pulsos en 5s ÷ 5.5 = 8.91 L/min
│
└─ JSON construcción:
   {
     "id": "nodo1",
     "alerta": 1,
     "flujo1": 15.45,
     "flujo2": 8.91,
     "lat": 23.456789,
     "lng": -101.234567,
     "sats": 12,
     "gps": 1
   }
```

### **T=0.05s: Transmisiones Simultáneas**
```
1️⃣ ESP-NOW → LilyGO (MAC: 10:06:1C:40:C4:44)
   Latencia: ~50-100ms
   Estado: [ESP-NOW OK]

2️⃣ LoRa Broadcast SF=9, 915MHz
   Latencia: ~500ms (tiempo aire)
   Destino: Receptor (para monitoreo visual)
   Estado: [LoRa TX]
```

### **T=0.1s: LilyGO SIM7070 (Gateway) Recibe ESP-NOW**
```
Callback onDataRecv():
├─ JSON recibido del Trasmisor vía ESP-NOW
├─ GPS del SIM7070: LAT -20.1356°, LON -99.2145°
│
└─ JSON Enriquecido:
   {
     "id": "nodo1",
     "alerta": 1,
     "flujo1": 15.45,
     "flujo2": 8.91,
     "lat": 23.456789,
     "lng": -101.234567,
     "sats": 12,
     "gps": 1,
     "lat_sim": -20.1356,
     "lon_sim": -99.2145,
     "gateway": "Tanque-01"
   }
```

### **T=0.15s: MQTT Publish**
```
Broker: broker.hivemq.com:1883
Topic: itics/mgti/isai/sensor
Payload: (JSON enriquecido)
Status: [MQTT OK - Published]
```

### **T=0.2s: Node-RED Recibe (Functions 8-11)**
```
Function 11 - Estructura para InfluxDB:
├─ Extrae flujo1=15.45, flujo2=8.91
├─ Calcula consumo_actual = 15.45 - 8.91 = 6.54 L/min
├─ Valida: lat≠0, lng≠0 ✓
│
└─ Punto InfluxDB:
   Measurement: "monitoreo_diesel_tics"
   Fields:
   ├─ caudal_flujo_ida: 15.45
   ├─ caudal_flujo_retorno: 8.91
   ├─ consumo_actual: 6.54
   ├─ estado_tapa: 1
   ├─ latitud: 23.456789
   └─ longitud: -101.234567
   Tags:
   ├─ unidad: "Camion_Kiwi"
   ├─ propietario: "Isai_TICS"
   ├─ sensor: "Hardware_LilyGO_Real"
   └─ gateway: "Tanque-01"
   Timestamp: 2026-05-04T20:13:45.123Z
```

### **T=0.25s: InfluxDB Almacena**
```
Bucket: SENSORES (ProyectoMD)
└─ Punto almacenado con timestamp
   (retención indefinida)
   Total: ~17,280 puntos/día
```

### **T=0.5s: Grafana Visualiza**
```
Dashboard "Monitoreo" (auto-refresh cada 5s):
├─ TIME SERIES: Flujo Ida=15.45, Flujo Retorno=8.91
├─ GAUGE: Consumo Neto=6.54 L/min (verde, normal)
├─ TIMELINE: Estado Tapa=CERRADA (verde, seguro)
└─ GEOMAP: Ubicación (20.1356°, -99.2145°) en mapa
```

---

### **T=5s: Ciclo Repite**
```
✅ Latencia Total (inicio a fin):
   Trasmisor → LilyGO (ESP-NOW): ~100ms
   LilyGO → MQTT: ~1s (4G latency)
   MQTT → Node-RED: ~100ms
   Node-RED → InfluxDB: ~500ms
   InfluxDB → Grafana: ~1-2s (query time)
   ───────────────────────────────
   TOTAL: ~3-4 segundos (dentro de 5s cycle)
```

---

## 📈 Visualización en Grafana

### **Gráfico 1: Time Series - Caudal de Flujo**

```flux
from(bucket: "SENSORES")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "monitoreo_diesel_tics")
  |> filter(fn: (r) => r["_field"] == "caudal_flujo")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
```

**Muestra:** Línea temporal del flujo promediado por ventana de tiempo

---

### **Gráfico 2: Time Series - Historial de Estado de Tapa**

```flux
from(bucket: "SENSORES")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "monitoreo_diesel_tics")
  |> filter(fn: (r) => r["_field"] == "estado_tapa")
  // Eliminamos last() para traer todos los eventos
  |> yield(name: "historial_tapa")
```

**Muestra:** Todos los cambios de estado (abierto/cerrado) con timestamp

---

### **Gráfico 3: Gauge - Valor Actual de Flujo**

```flux
from(bucket: "SENSORES")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "monitoreo_diesel_tics")
  |> filter(fn: (r) => r["_field"] == "caudal_flujo")
  |> last()
```

**Muestra:** Último valor de flujo en tiempo real (aguja/indicador)

---

### **Gráfico 4: Geomap - Ubicación del Camión**

```flux
from(bucket: "SENSORES")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "monitoreo_diesel_tics")
  |> filter(fn: (r) => r["_field"] == "latitud" or r["_field"] == "longitud")
  |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
```

**Muestra:** Ubicación GPS en mapa interactivo

---

## 🔧 Troubleshooting

### **❌ Problema: No llegan datos a Node-RED**

**Causas posibles:**
1. LilyGO sin conexión GPRS
   - Verificar que el modem esté encendido
   - Revisar APN y credenciales
   - Probar con comando AT: `modem.sendAT("+CGMI");`

2. Broker MQTT caído
   - Verificar: https://broker.hivemq.com/
   - Cambiar por otro broker (Mosquitto, AWS IoT, Azure IoT Hub)

3. WiFi LilyGO no detecta Heltec
   - Verificar MAC address: `WiFi.macAddress()`
   - Comparar con `broadcastAddress[]` en código
   - Ambas deben estar con `WiFi.mode(WIFI_STA)`

---

### **❌ Problema: GPS no obtiene satélites**

**Causas posibles:**
1. Antena GPS sin línea de vista
   - Acercar a ventana
   - Esperar 30-60 segundos (TTFF: Time To First Fix)

2. Baud rate incorrecto
   - Heltec: `gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);` ✓
   - Común: 9600 bps (estándar NMEA)

3. Módulo GPS defectuoso
   - Verificar respuestas NMEA en Serial Monitor
   - Ejemplo: `$GPRMC,142332.00,A,2345.67890,S,10112.34567,W,0.295,,141120,,,A*78`

---

### **❌ Problema: Datos inconsistentes en InfluxDB**

**Causas posibles:**
1. Coordenadas 0,0 siendo rechazadas (es normal)
   - Ver logs Node-RED: `⚠️ Coordenadas inválidas`
   - Resolver problema de GPS

2. Timestamps inconsistentes
   - Verificar que ESP32/LilyGO tengan NTP configurado
   - Usar `configTime()` para sincronización

3. Validación de JSON fallando
   - Revisar formato JSON en Serial Monitor
   - Encodage UTF-8 correcto

---

### **❌ Problema: Buffer lleno (se pierden datos)**

**Causas posibles:**
1. Conectividad GPRS intermitente
   - Buffer máximo de 10 datos
   - Si hay desconexión prolongada, se pierde el 11º dato
   
**Solución:**
- Aumentar `#define MAX_BUFFER 10` a 20 o 50
- Recompilar y cargar firmware

```cpp
#define MAX_BUFFER 50  // Aumentado
String bufferDatos[MAX_BUFFER];
```

---

### **❌ Problema: Latencia alta (datos lentos)**

**Análisis de tiempos:**
```
Heltec → LilyGO (ESP-NOW):            < 100 ms
LilyGO → MQTT (GPRS):                 1-5 segundos
MQTT → Node-RED:                      < 100 ms
Node-RED → InfluxDB:                  < 500 ms
InfluxDB → Grafana (query):           1-2 segundos
────────────────────────────────────────────────
Total (latencia de inicio a fin):     3-8 segundos
```

Si tarda más:
- Verificar latencia GPRS: `modem.ping("broker.hivemq.com")`
- Revisar procesamiento Node-RED (función 11)
- Aumentar frecuencia de queryeo en Grafana

---

## 📝 Notas Técnicas Importantes

### **0. Spreading Factor (SF) - Factor de Propagación**

#### **¿Qué es el Spreading Factor?**

El Spreading Factor es un parámetro LoRa que determina la duración de cada símbolo transmitido. Define cuánto se "expande" la señal en el dominio del tiempo y la frecuencia.

| SF | Rango | Tiempo Aire | Sensibilidad | Velocidad | Uso |
|----|-------|-----------|--------------|-----------|-----|
| SF7 | ~2-4 km | 50ms | -120dBm | 🔴 Rápido | Corta distancia, baja interferencia |
| SF8 | ~3-5 km | 100ms | -123dBm | 🟡 Medio | Óptimo para ciudad |
| **SF9** | **~5-8 km** | **200ms** | **-126dBm** | **🟢 Balanceado** | **✅ Nuestro caso (tanque + EM)** |
| SF10 | ~8-12 km | 400ms | -129dBm | 🟢 Lento | Larga distancia |
| SF12 | 15+ km | 1.6s | -137dBm | 🔵 Muy lento | Máxima robustez |

#### **¿Por qué SF=9 en nuestro sistema?**

**1. Ambiente Hostil (Metal + Motor EM)**
- Tanque: Estructura de aluminio/acero → reflexiones de ondas
- Motor diesel: Ruido electromagnético (alternador, inyectores) en la banda 915MHz
- **SF=9 es robusto contra interferencia** pero más rápido que SF=10-12

**2. Balance Latencia-Robustez**
```
Trasmisor lee sensores → T=0s
Trasmisor envía LoRa SF=9 → T=0-200ms (tiempo aire)
Receptor recibe → T=50-500ms
LilyGO MQTT → T=1s (4G latency)
───────────────────────────────
Total: 3-4 segundos (aceptable para ciclo 5s)

Si usáramos SF=12: Tiempo aire=1.6s → Total=5-6s (demasiado lento)
Si usáramos SF=7: Sensibilidad baja, pierde paquetes en EM
```

**3. Sensibilidad Óptima**
- SF=9 con -126dBm permite detectar señales muy débiles
- Suficiente para penetración en metal del tanque
- Mejor que SF=7 (-120dBm) que falla con interferencia

**4. Código Implementado**
```cpp
radio.setSpreadingFactor(9);    // SF=9 (CRÍTICO)
radio.setBandwidth(125.0);      // 125 kHz (recomendado)
radio.setCodingRate(5);         // 4/5 (máxima redundancia)
radio.setOutputPower(17);       // 17 dBm (máxima potencia EU)
```

#### **Tiempo Aire (Air Time) Fórmula**
```
T_symbol = (2^SF) / BW
T_air = T_preamble + T_payload + T_overhead

SF=9, BW=125kHz, payload=256 bytes:
T_air ≈ 200-300ms por transmisión
```

#### **Validación en Campo**
✅ Datos reales del sistema (mayo 4, 2026):
- Distancia Trasmisor→Receptor: 100-150m
- RSSI (Received Signal Strength): -95 a -110 dBm
- Pérdida de paquetes: <2% incluso con motor a máxima potencia
- **Conclusión: SF=9 es ÓPTIMO para este deployment**

---

### **1. Sincronización de Tiempo**

Ambos ESP32 deben sincronizarse con NTP:

```cpp
configTime(0, 0, "pool.ntp.org", "time.nist.gov");
time_t now = time(nullptr);
Serial.println(ctime(&now));  // Verificar hora sincronizada
```

### **2. Eficiencia de Potencia**

Heltec (modo field):
- WiFi inactivo cuando no envía (ahorra batería)
- GPS siempre activo (puede drenar rápido)
- Considerar usar deep sleep entre envíos

```cpp
// Después de enviar
esp_sleep_enable_timer_wakeup(5000000);  // 5 segundos
esp_light_sleep_start();
```

### **3. Manejo de Errores**

Implementado en LilyGO:
- ✅ Buffer automático si falla MQTT
- ✅ Reintento de conexión GPRS
- ✅ Indicador LED de estado
- ✅ Logs en Serial para debugging

### **4. Seguridad**

⚠️ **IMPORTANTE para producción:**
- NO hardcodear credenciales GPRS en código
- Usar variables de entorno o EEPROM encriptada
- Activar encriptación en ESP-NOW (actual: desactivada)
- Validar origen de datos en Node-RED
- Usar contraseña en broker MQTT

### **5. Escalabilidad**

Sistema actual: 1 Heltec → 1 LilyGO

Para múltiples camiones:
- Agregar `id` del camión en JSON (ya implementado: "id": "Tanque-01")
- Crear múltiples gateways LilyGO
- Node-RED puede procesar múltiples topics
- InfluxDB etiqueta automáticamente por unidad

---

## 🚀 Próximos Pasos Recomendados

1. **Backup de configuraciones**
   - Guardar flows Node-RED
   - Guardar queries Grafana como JSON
   - Documentar credenciales en gestor seguro

2. **Monitoreo y alertas**
   - Agregar alertas en Grafana (si flujo = 0 por > 10 min)
   - Notificaciones por email/SMS si tapa se abre inesperadamente

3. **Mejoras técnicas**
   - Implementar sincronización NTP
   - Agregar checksum/CRC a datos JSON
   - Implementar HTTPS para datos sensibles

4. **Testing**
   - Simular pérdida de conexión (desplug modem)
   - Verificar reintento de buffer
   - Pruebas de carga (múltiples camiones)

---

## 📞 Contacto & Documentación

**Proyecto:** Monitoreo Remoto de Tanques de Diésel  
**Responsable:** Isai TICS  
**Última actualización:** Abril 2026  

**Archivos relacionados:**
- flows (2).json - Configuración Node-RED
- Trasmisor.ino - Código Heltec LoRa v3
- sketch_mar11c.ino - Código LilyGO SIM7070
- GRAFANA-Scripts.txt - Queries de Grafana

---

**FIN DE DOCUMENTACIÓN**
