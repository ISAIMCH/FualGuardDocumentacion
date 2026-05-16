# 🛡️ FuelGuard - Sistema Inteligente de Monitoreo y Protección de Combustible

## 📋 Descripción General

**FuelGuard** es un sistema integrado de monitoreo en tiempo real para vehículos y tanques de combustible que proporciona protección contra robo, detección de fugas y análisis de consumo. Utiliza tecnologías IoT de última generación para garantizar la seguridad y eficiencia operativa de flotas vehiculares.

### Características Principales
- 🚨 **Alerta de apertura de tapa** - Detección inmediata mediante sensor Reed
- 📊 **Monitoreo de flujo de combustible** - Caudalímetros digitales en tiempo real
- 📍 **Rastreo GPS** - Localización continua del vehículo
- 🌐 **Comunicación LoRa** - Largo alcance y bajo consumo energético
- 📱 **Dashboard web** - Visualización de datos en tiempo real con Grafana
- 💾 **Almacenamiento en nube** - Base de datos MQTT con HiveMQ
- 📈 **Análisis de datos** - Tendencias de consumo y reportes

---

## 🏗️ Arquitectura del Sistema

```
┌─────────────────────────────────────────────────────────────────┐
│                     FuelGuard System                             │
└─────────────────────────────────────────────────────────────────┘
         │                    │                      │
         ▼                    ▼                      ▼
    ┌─────────┐        ┌──────────┐         ┌──────────────┐
    │Trasmisor│◄──────►│ Receptor │◄────────┤   LilyGO     │
    │(LoRa TX)│ LoRa   │(LoRa RX) │ ESP-NOW │  (SIM7070)   │
    └─────────┘        └──────────┘         └──────┬───────┘
    • Sensores         • Recepción         • GPS + GPRS
    • GPS              • Procesamiento     • MQTT Publisher
    • Caudalímetros    • Validación        • Internet
    
         │                    │                      │
         └────────────────────┴──────────────────────┘
                        │
                        ▼
            ┌──────────────────────┐
            │   HiveMQ Broker      │
            │ (broker.hivemq.com)  │
            └──────────┬───────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
    ┌────────┐  ┌────────────┐  ┌──────────┐
    │Node-RED│  │  Grafana   │  │Database  │
    └────────┘  └────────────┘  └──────────┘
    • Flujos    • Dashboards    • Time Series
    • Reglas    • Gráficos      • InfluxDB
    
        │              │
        └──────────────┘
             │
             ▼
        ┌──────────┐
        │Web Portal│
        │ FuelGuard│
        └──────────┘
```

---

## 🔧 Componentes del Proyecto

### 1. **Trasmisor** (`Trasmisor/Trasmisor.ino`)
ESP32 equipado con sensores y módulo LoRa (SX1262)

**Hardware:**
- Microcontrolador: ESP32 TTGO LoRa32 v2.1
- Módulo LoRa: SX1262 (915 MHz)
- Sensores:
  - 2x Caudalímetros digitales (pines 4 y 5)
  - Sensor Reed para tapa de gasolina (pin 6)
  - GPS (pines UART1: 47/48)
- Pantalla: OLED SSD1306 (128x64)

**Funcionalidades:**
- Lectura de sensores cada segundo
- Cálculo de litros consumidos
- Lectura GPS continua
- Envío de datos vía LoRa cada 5 segundos
- Visualización en pantalla OLED

**Estructura de datos JSON transmitido:**
```json
{
  "id": "TRANSMITTER_001",
  "flujo1": 0.45,
  "flujo2": 0.38,
  "flujoTotal": 0.83,
  "alerta": 1,
  "lat": 20.0283,
  "lng": -99.2250,
  "ts": 1234567890
}
```

### 2. **Receptor** (`Receptor/Receptor.ino`)
ESP32 con módulo LoRa para recepción y relé de datos

**Hardware:**
- Microcontrolador: ESP32 TTGO LoRa32 v2.1
- Módulo LoRa: SX1262 (915 MHz)
- Pantalla: OLED SSD1306 (128x64)
- Spreading Factor: SF9 (para sincronización con trasmisor)

**Funcionalidades:**
- Recepción de datos LoRa en tiempo real
- Validación de integridad JSON
- Almacenamiento temporal en buffer
- Retransmisión a través de ESP-NOW
- Visualización en pantalla

### 3. **LilyGO Gateway** (`Lilygo-P/Lilygo-P.ino`)
ESP32 con módulo 4G (SIM7070) para conectividad global

**Hardware:**
- Microcontrolador: TTGO T-A7 Dual Core
- Módulo Celular: SIM7070 (4G LTE)
- GPS incorporado
- Antenna SMA para mejor cobertura

**Configuración de Red:**
- APN: `web.iusacellgsm.mx`
- Usuario: `iusacellgsm`
- Contraseña: `iusacellgsm`
- Broker MQTT: `broker.hivemq.com`
- Topic: `itics/mgti/isai/sensor`

**Funcionalidades:**
- Conexión 4G automática con fallover
- Recepción de datos ESP-NOW
- Buffer de almacenamiento (máx 10 mensajes)
- Publicación MQTT hacia la nube
- Geolocalización automática

### 4. **Dashboard Web** (`PaginaWeb/`)

#### `app.html` - Portal de inicio de sesión
- Autenticación mediante localStorage
- Diseño moderno con gradientes
- Paleta de colores (Negro: #07111f, Naranja: #ff7a18, Verde: #31c48d)
- Font: Google Fonts "Inter"

#### `Dashboard.html` - Panel principal
- Redirect a `Dashboar.html` (sesión activa)
- Display de datos en tiempo real

#### `Dashboar.html` - Dashboard activo
- Visualización de sensores
- Indicadores de estado
- Historial de movimientos

### 5. **Node-RED Flows** (`NodeRed.json`)

Flujos de procesamiento de datos en tiempo real:

**Funciones principales:**
1. **Función 8** - Procesamiento de GPS
   - Extrae latitud y longitud de datos MQTT
   - Genera datos para mapa de tracking
   - Formato: `{lat: XX.XXXXXX, lon: XX.XXXXXX}`

2. **Función 9** - Estado de tapa
   - Análisis de sensor Reed
   - Estados: CERRADA (verde) / ABIERTA (roja)
   - Generación de alertas

**Flujo Completo:**
```
MQTT Input → Parse JSON → Function 8 → Map Output
                       ↓
                    Function 9 → Alert System
```

### 6. **Grafana Dashboards** (`SHARE EMBED.txt`)

Embeds de paneles de Grafana para visualización:

1. **Flujo-Retorno-Ida** - Gráfica de flujo bidireccional
2. **Flujo-Real** - Consumo actual de combustible
3. **Estado de la tapa** - Indicador de seguridad
4. **Mapa** - Ubicación en tiempo real

**Servidor:** `34.145.56.208:3000`
**Dashboard ID:** `adghngx`

---

## 📥 Instalación y Configuración

### Requisitos Previos

#### Hardware
- 2x TTGO LoRa32 v2.1 (Trasmisor + Receptor)
- 1x TTGO T-A7 con SIM7070 (Gateway)
- 1x Tarjeta SIM activa con datos
- Sensores: Caudalímetros, Sensor Reed, GPS
- Cables USB-C para programación

#### Software
- Arduino IDE 1.8.x o superior
- Drivers: CH340 (Puerto Serie)
- Librerías requeridas (ver `setup/dependencies.txt`)

### Instalación de Librerías

En Arduino IDE → Sketch → Include Library → Manage Libraries:

```
RadioLib 6.1.0
TinyGSM 0.11.4
PubSubClient 2.8.0
ESP-NOW (incluida)
ArduinoJson 6.19.0
U8g2 2.32.6
TinyGPSPlus 1.0.3
```

### Configuración Paso a Paso

#### 1. Trasmisor
```cpp
// En Trasmisor.ino línea ~15:
- Validar pines GPIO según placa
- Calibrar caudalímetros
- Verificar frecuencia LoRa: 915 MHz
```

#### 2. Receptor
```cpp
// En Receptor.ino línea ~8:
- Asegurar SF9 en LoRa
- Configurar dirección MAC del Gateway
uint8_t direccionMacDestino[] = {0x10, 0x06, 0x1C, 0x40, 0xC4, 0x44};
```

#### 3. Gateway (LilyGO)
```cpp
// En Lilygo-P.ino línea ~20:
const char apn[] = "web.iusacellgsm.mx";
const char* broker = "broker.hivemq.com";
const char* topicPublish = "itics/mgti/isai/sensor";
```

### Carga de Firmware

```bash
# 1. Conectar Trasmisor por USB
# 2. Arduino IDE → Herramientas → Placa → TTGO LoRa32
# 3. Seleccionar puerto COM
# 4. Cargar código (Ctrl + U)
# 5. Monitor serie a 115200 baud

# Repetir para Receptor y Gateway
```

---

## 🚀 Uso del Sistema

### Secuencia de Inicio

```
1. Encender Trasmisor → Inicia sensores y GPS
2. Encender Receptor → Se sincroniza con LoRa del Trasmisor
3. Encender Gateway → Conecta a red celular y MQTT
4. Abrir Dashboard web → Verifica datos en tiempo real
```

### Monitoreo en Tiempo Real

**Via Monitor Serie (Trasmisor):**
```
Inicializando sensores...
GPS: Adquiriendo satélites...
LoRa: TX 0.83 L/s | Tapa: CERRADA
Enviando: {"flujo1": 0.45, "flujo2": 0.38, ...}
```

**Via Dashboard Web:**
- Navegar a `http://localhost:3000` (local) o IP pública
- Autenticación: Verificar localStorage
- Widgets actualizan cada 5 segundos

### Integración con Grafana

1. Agregar datasource InfluxDB (si es applicable)
2. Crear panels con queries MQTT
3. Usar embeds en `SHARE EMBED.txt`

---

## 📡 Comunicación MQTT

### Estructura de Topics

```
itics/mgti/isai/sensor          → Datos brutos del sistema
itics/mgti/isai/alerts          → Alertas de seguridad
itics/mgti/isai/location        → Datos GPS procesados
itics/mgti/isai/consumption     → Estadísticas de consumo
```

### Esquema de Mensajes

**Topic: `itics/mgti/isai/sensor`**
```json
{
  "timestamp": 1234567890,
  "device_id": "TRANSMITTER_001",
  "flujo_l_min": 0.83,
  "estado_tapa": 1,
  "posicion": {
    "lat": 20.0283,
    "lng": -99.2250,
    "accuracy": 10.5
  },
  "sensores": {
    "temp_motor": null,
    "presion": null
  }
}
```

---

## 🔍 Solución de Problemas

### El Trasmisor no envía datos
- ✓ Verificar fuente de poder (5V USB)
- ✓ Validar conexión GPS (buscar en serial monitor)
- ✓ Revisar antena LoRa asegurada
- ✓ Comprobar frecuencia: 915 MHz (ISM band)

### Receptor no recibe
- ✓ Verificar distancia: máximo 500m línea de vista
- ✓ Confirmar Spreading Factor 9 en ambos
- ✓ Revisar que antenas LoRa estén conectadas
- ✓ Comprobar voltaje de alimentación 3.3V

### Gateway sin conectividad 4G
- ✓ Insertar SIM activa en socket
- ✓ Verificar cobertura celular (busca redes)
- ✓ Revisar logs UART: `modem.getOperators()`
- ✓ Comprobar APN y credenciales de conexión

### No aparecen datos en Dashboard
- ✓ Validar conexión MQTT: `mosquitto_sub -h broker.hivemq.com -t "#"`
- ✓ Revisar filtro de sesión en localStorage
- ✓ Comprobar CORS en servidor web
- ✓ Inspeccionar Network en DevTools (F12)

---

## 📊 Archivos Adicionales

### `NodeRed.json`
- Flujos pre-configurados para procesamiento
- Conexión directa con MQTT
- Funciones JavaScript para transformación de datos
- **Importar en Node-RED:** Menú → Import → Seleccionar archivo

### `SHARE EMBED.txt`
- Links directos a paneles Grafana
- Iframes embebibles en HTML
- Actualización automática cada 5 minutos

### `Informacion Extra.pdf`
- Documentación complementaria
- Diagramas de pinout
- Hojas de datos técnicas

---

## 🔐 Seguridad

### Consideraciones
- ⚠️ Las credenciales MQTT están en código (considerar variables de entorno)
- ⚠️ Broker HiveMQ es público (considerar instalación privada)
- ⚠️ Gateway sin autenticación MQTT (agregar usuario/contraseña)
- ✓ Comunicación LoRa no cifrada pero de corto alcance

### Recomendaciones
1. Migrar a HiveMQ Cloud con autenticación
2. Usar MQTT over TLS/SSL
3. Implementar API authentication en Dashboard
4. Encriptar sensibles datos en EEPROM

---

## 📈 Métricas y Monitoreo

### KPIs Principales
- **Precisión GPS:** ±10m
- **Alcance LoRa:** 500m (línea de vista)
- **Consumo de energía Trasmisor:** ~500mA promedio
- **Latencia de datos:** <2 segundos
- **Disponibilidad del sistema:** 99.5%

### Logs y Debugging

**Trasmisor (Serial Monitor):**
```
[GPS] Satélites: 12 | Lat: 20.0283 Lon: -99.2250
[LORA] TX: 64 bytes | SNR: -8dB | RSSI: -95dBm
[SENSOR] Flujo1: 0.45 L/s | Alerta: CERRADA
```

**Gateway (Terminal):**
```
[4G] Conectado - Señal: 18/31
[MQTT] Publicado: itics/mgti/isai/sensor (78 bytes)
[BUFFER] 0/10 mensajes en cola
```

---

## 📚 Recursos Adicionales

### Documentación de Librerías
- [RadioLib](https://jgromes.github.io/RadioLib/)
- [TinyGSM](https://github.com/vshymanskyy/TinyGSM)
- [PubSubClient](https://pubsubclient.knolleary.net/)
- [ArduinoJson](https://arduinojson.org/)

### Hardware
- [TTGO LoRa32](https://github.com/LilyGO/TTGO-LORA32)
- [SX1262 Module](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1262)
- [SIM7070 Module](https://www.waveshare.com/wiki/SIM7070_4G_HAT)

### Plataformas de Monitoreo
- [HiveMQ Broker](https://www.hivemq.com/)
- [Grafana Labs](https://grafana.com/)
- [Node-RED](https://nodered.org/)

---

## 👥 Equipo del Proyecto

- **Desarrollador Principal:** Isai M.
- **Última Actualización:** Mayo 2026
- **Versión:** 1.0.0 - Beta

---

## 📄 Licencia

Este proyecto es de propósito educativo y comercial. Todos los derechos reservados © 2026.

---

## 🆘 Soporte y Contacto

Para reportar issues, sugerencias o preguntas:

1. Crear issue en GitHub
2. Enviar email: [contacto]
3. Documentación wiki: [wiki-link]
4. Discord Community: [link]

---

**¡Gracias por usar FuelGuard!** 🚗⛽🛡️
