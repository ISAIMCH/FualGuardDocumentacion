# Especificaciones Técnicas

## 🔌 Hardware Pinout

### TTGO LoRa32 v2.1 (Trasmisor & Receptor)

```
       ┌─────────────────────┐
       │   TTGO LoRa32 v2.1  │
       │                     │
    GND├─────────────────────┤3V3
    RST├─────────────────────┤GND
    3V3├─────────────────────┤VP
     IO15├─────────────────────┤VN
     IO13├─────────────────────┤IO14
     IO12├─────────────────────┤IO2
     IO17├─────────────────────┤IO4  ← MODEM_PWRKEY
     IO16├─────────────────────┤IO5
     IO27├─────────────────────┤IO18 ← OLED_SCL
     IO26├─────────────────────┤IO19
     IO25├─────────────────────┤IO23 ← SPI_CLK
     IO32├─────────────────────┤IO22
     IO35├─────────────────────┤IO21 ← OLED_RST
     IO34├─────────────────────┤IO17 ← OLED_SDA
     IO33├─────────────────────┤IO16
       ├─────────────────────┤
       │     USB-C           │
       └─────────────────────┘
```

#### Asignación de Pines

**OLED Display (SSD1306)**
| Función | Pin | Notas |
|---------|-----|-------|
| SDA | 17 | I2C Data |
| SCL | 18 | I2C Clock |
| RST | 21 | Reset |
| VCC | 3V3 | Alimentación |
| GND | GND | Tierra |

**LoRa SX1262**
| Función | Pin | Notas |
|---------|-----|-------|
| SCK | 9 | SPI Clock |
| MOSI | 10 | SPI Master Out |
| MISO | 11 | SPI Master In |
| CS | 8 | Chip Select |
| DIO1 | 14 | Data In/Out |
| RST | 12 | Reset |
| BUSY | 13 | Busy Line |
| GND | GND | Tierra |
| 3V3 | 3V3 | Alimentación |

**Sensores (Trasmisor)**
| Función | Pin | Tipo | Notas |
|---------|-----|------|-------|
| Flujo 1 | 4 | Digital | Caudalímetro |
| Flujo 2 | 5 | Digital | Caudalímetro |
| Reed | 6 | Digital | Sensor tapa |
| GPS TX | 47 | UART | Entrada de datos |
| GPS RX | 48 | UART | Salida de datos |

**Potencia**
| Función | Pin | Voltaje | Notas |
|---------|-----|---------|-------|
| VEXT | 36 | 3.3V | Control de energía periféricos |

---

## 📊 Especificaciones Eléctricas

### Trasmisor & Receptor
```
Voltaje: 3.3V - 5V (USB)
Corriente en operación: 400-500mA
Corriente en reposo: 50mA
Consumo pico: 1.2A (transmisión LoRa)
Rango operativo: -10°C a +50°C
```

### Gateway LilyGO T-A7
```
Voltaje: 5V (USB)
Corriente: 800mA - 2A (4G activo)
Banda 4G: LTE CAT-M1 / NB-IoT
Sensibilidad: -120dBm @ SF12
Ganancia antena: 2dBi
```

---

## 📡 Parámetros LoRa

### Configuración Principal

```
Frecuencia: 915 MHz (ISM Band)
Ancho de banda: 125 kHz
Spreading Factor: 9 (SF9)
Coding Rate: 4/7
Preamble: 8
CRC: Habilitado
IQ: Invertido
```

### Rendimiento

| Parámetro | Valor |
|-----------|-------|
| Tiempo de transmisión | ~2-3 segundos |
| Tiempo de recepción | <1 segundo |
| Alcance máximo | 500m línea vista |
| Throughput | 1-5 kbps |
| Poder RX mín | -120 dBm |

---

## 🌐 Configuración de Red

### Broker MQTT HiveMQ

**Servidor:** `broker.hivemq.com`
**Puerto:** 1883 (TCP) | 8883 (TLS)
**Protocolo:** MQTT v3.1.1 / v5.0

**Topics:**
```
itics/mgti/isai/sensor          → Data stream principal
itics/mgti/isai/alerts          → Alertas de seguridad
itics/mgti/isai/location        → Datos GPS procesados
itics/mgti/isai/consumption     → Estadísticas consumo
```

### APN Celular (Iusacell)

```
APN: web.iusacellgsm.mx
Usuario: iusacellgsm
Contraseña: iusacellgsm
Autenticación: PAP
Tipo IP: IPv4v6
```

---

## 📦 Estructura de Datos

### Payload JSON Trasmisor

```json
{
  "id": "TRANSMITTER_001",
  "timestamp": 1234567890,
  "sensores": {
    "flujo1_lps": 0.45,
    "flujo2_lps": 0.38,
    "flujo_total": 0.83,
    "tapa_estado": 1,
    "temperatura": null,
    "presion": null
  },
  "ubicacion": {
    "latitud": 20.028306,
    "longitud": -99.225007,
    "altitud": 2250.5,
    "velocidad": 45.3,
    "rumbo": 180,
    "precision": 10.5,
    "satelites": 12
  },
  "bateria": {
    "voltaje": 4.15,
    "porcentaje": 85,
    "corriente": 450
  },
  "sistema": {
    "uptime": 3600,
    "rssi_lora": -95,
    "snr": -8,
    "version_fw": "1.0.0"
  }
}
```

### Payload Node-RED → Dashboard

**GPS Processing:**
```json
{
  "name": "Unidad001",
  "lat": 20.028306,
  "lon": -99.225007,
  "icon": "truck",
  "iconColor": "#FFC107",
  "layer": "Unidades",
  "timestamp": 1234567890
}
```

**Reed Sensor:**
```json
{
  "estado": "CERRADA",
  "background": "green",
  "timestamp": 1234567890
}
```

---

## 🔄 Ciclos de Actuación

### Trasmisor

```
00:00 - 00:01   Lectura de sensores (cada 100ms)
00:00 - 00:05   Adquisición GPS continua
00:05 - 00:10   Transmisión LoRa (5 segundos)
...
Ciclo se repite cada 5 segundos
```

### Gateway

```
Recepción ESP-NOW: Cada 5 segundos
Conexión 4G: Cada 60 segundos (verificar)
Publicación MQTT: Inmediata al recibir
Buffer: Máximo 10 mensajes
```

---

## 🧮 Fórmulas y Cálculos

### Caudalímetro

```
Litros/segundo = (Pulsos / PPL) / Δt

Donde:
- Pulsos = impulsos contados
- PPL = Pulsos Por Litro (calibrar por sensor)
- Δt = Intervalo de tiempo (segundos)

Típicamente:
- PPL ≈ 5880 para caudalímetros comunes
- Lectura cada 1 segundo
```

### GPS Precision

```
Error horizontal ≈ ±10 metros (95% confianza)
Error vertical ≈ ±20 metros
Velocidad mínima detectable: ~0.1 m/s (0.36 km/h)
```

### Consumo de Energía

```
Trasmisor modo transmisión:
- Lectura sensores: 50mA
- LoRa TX: 500mA
- GPS activo: 80mA
- Total: ~600mA (picos)

Promedio: ~450mA
Batería 2500mAh → ~5.5 horas autonomía
```

---

## 🔍 Monitoreo y Métricas

### Comandos AT del Módem SIM7070

```
AT                          → Test conexión
AT+CSQ                      → Calidad de señal
AT+CGPS?                    → Estado GPS
AT+CMQTTSTART              → Iniciar MQTT
AT+CMQTTPUB=1,0,0          → Publicar
```

### RSSI/SNR Reference

```
RSSI (dBm):
-30 to -85 = Excelente
-85 to -100 = Bueno
-100 to -120 = Marginal
< -120 = No detectable

SNR (dB):
> 10 = Excelente
5 to 10 = Muy bien
0 to 5 = Bien
< 0 = Débil
```

---

## 🔧 Calibración

### Caudalímetro

```cpp
// Método de calibración:
// 1. Llenar contenedor de volumen conocido
// 2. Contar pulsos durante el llenado
// 3. Calcular: PPL = Pulsos / Volumen_litros
// 4. Actualizar en código:

#define PPL_FLUJO1 5880
#define PPL_FLUJO2 5880
```

### GPS

```cpp
// Verificar precisión:
// 1. Posicionar en ubicación conocida (Google Maps)
// 2. Comparar con coordenadas recibidas
// 3. Validar offset si es necesario
// 4. Precision típica: ±10m

float lat_correction = 0.0;
float lon_correction = 0.0;
```

---

## 📚 Referencias

- [ESP32 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [SX1262 Datasheet](https://www.semtech.com/uploads/documents/DS_SX1261-2_V1.2.pdf)
- [LoRa Modulation Basics](https://lora-alliance.org/about-lorawan/)
- [MQTT 3.1.1 Spec](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html)

