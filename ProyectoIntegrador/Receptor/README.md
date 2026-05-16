# Receptor - ESP32 LoRa Relay

## 📡 Descripción

Unidad receptora que captura transmisiones LoRa del trasmisor y las retransmite mediante ESP-NOW hacia el gateway LilyGO.

## 🎯 Función Principal

Actuar como repetidor/relay de datos LoRa → ESP-NOW para mejorar alcance y cobertura del sistema.

## 📦 Contenido

```
Receptor/
├── Receptor.ino           ← Código principal
└── README.md              ← Este archivo
```

## 🔧 Hardware

| Componente | Especificación |
|-----------|----------------|
| Microcontrolador | TTGO LoRa32 v2.1 |
| Módulo LoRa | SX1262 (Idéntico Trasmisor) |
| Pantalla | OLED SSD1306 128x64 |

## 🚀 Instalación

```bash
# 1. Abrir Receptor.ino en Arduino IDE
# 2. Configurar dirección MAC del Gateway (línea 8)
# 3. Cargar en placa Receptor
# 4. Verificar sincronización en serial monitor
```

## 🔗 Configuración ESP-NOW

**IMPORTANTE:** Cambiar dirección MAC destino

```cpp
// Línea 8 en Receptor.ino:
uint8_t direccionMacDestino[] = {0x10, 0x06, 0x1C, 0x40, 0xC4, 0x44};
// ↑ Reemplazar con MAC real del Gateway
```

**Cómo obtener MAC del Gateway:**
```cpp
// Compilar y ejecutar LilyGO-P.ino
// Serial monitor mostrará: "MAC LilyGO: XX:XX:XX:XX:XX:XX"
```

## 🔄 Flujo de Datos

```
Trasmisor
   ↓ (LoRa 915MHz)
Receptor
   ├─ Recepción
   ├─ Validación JSON
   ├─ Almacenamiento buffer
   └─ Retransmisión
        ↓ (ESP-NOW)
      Gateway (LilyGO)
        ↓
      MQTT HiveMQ
```

## 📥 Datos Recibidos

```json
{
  "id": "TRANSMITTER_001",
  "flujo1_lps": 0.45,
  "flujo2_lps": 0.38,
  "alerta_tapa": 1,
  "lat": 20.028306,
  "lng": -99.225007
}
```

## 🔌 Pines GPIO

Idénticos al Trasmisor (ver [Trasmisor/README.md](../Trasmisor/README.md))

```
LoRa SX1262:
├── SCK: GPIO 9
├── MOSI: GPIO 10
├── MISO: GPIO 11
├── CS: GPIO 8
├── DIO1: GPIO 14
├── RST: GPIO 12
└── BUSY: GPIO 13

OLED:
├── SDA: GPIO 17
├── SCL: GPIO 18
└── RST: GPIO 21
```

## 📝 Configuración

Editar línea 8-20:

```cpp
// MAC destino del Gateway LilyGO
uint8_t direccionMacDestino[] = {0x10, 0x06, 0x1C, 0x40, 0xC4, 0x44};

// Spreading Factor (DEBE ser igual al Trasmisor)
#define LORA_SF 9              // Importante: SF9

// Frecuencia LoRa
#define LORA_FREQ 915.0        // MHz
```

## 🧪 Testing

### Serial Monitor (115200 baud)

```
[SETUP] Inicializando Receptor...
[ESP-NOW] Asociado a Gateway
[LORA] Módulo SX1262 OK
[OLED] Pantalla activada

[OLED] Escuchando LoRa...
[OLED] SF9: EN LINEA

[LORA] Paquete recibido: 64 bytes
[JSON] {"flujo1": 0.45, ...}
[BUFFER] 1/10 mensajes en cola
[ESP-NOW] Enviando a Gateway...
[ESP-NOW] Entregado

[OLED] Datos recibidos
[OLED] Última actualización: 12:34:56
```

### Verificación en Campo

1. **Cercano (50m):**
   - Trasmisor debe transmitir
   - Receptor recibe inmediatamente
   - RSSI: -70 a -80 dBm

2. **Distancia media (200m):**
   - Pequeño delay (<1 segundo)
   - RSSI: -90 a -100 dBm

3. **Distancia máxima (500m):**
   - Delay variable
   - Posibles retransmisiones
   - RSSI: -100 a -120 dBm

## 📊 Buffer de Datos

```cpp
// Receptor almacena hasta 10 mensajes
#define MAX_BUFFER 10

// Estructura:
struct BufferMsg {
  char json[256];
  uint32_t timestamp;
};

BufferMsg bufferDatos[MAX_BUFFER];
int bufferIndex = 0;
```

## 🆘 Troubleshooting

### "No sincroniza LoRa"
- [ ] Trasmisor transmitiendo?
- [ ] SF es 9 en ambos?
- [ ] Frecuencia 915 MHz?
- [ ] Antenas conectadas?
- [ ] Distancia < 500m?

### "ESP-NOW no conecta"
- [ ] MAC del Gateway correcto?
- [ ] Gateway encendido?
- [ ] Ambos en modo WiFi_STA?
- [ ] Revisar pairing en serial

### "Buffer lleno, datos perdidos"
- [ ] Gateway recibiendo?
- [ ] Velocidad transmisión muy rápida?
- [ ] Aumentar intervalo en Trasmisor?
- [ ] Aumentar MAX_BUFFER a 20?

## 📈 Rendimiento

| Métrica | Valor |
|---------|-------|
| Latencia LoRa | <3 segundos |
| Latencia ESP-NOW | <100 ms |
| Overhead JSON | ~64 bytes |
| Alcance LoRa | 500m línea vista |
| Alcance ESP-NOW | 250m (adicional) |

## ⚠️ Limitaciones

1. **No procesa datos** - Solo retransmite
2. **No filtra duplicados** - Podría causar loops
3. **Buffer limitado** - Máx 10 mensajes
4. **Una dirección destino** - No multicast

## 🔐 Seguridad

- LoRa sin encriptación (corto alcance como protección)
- ESP-NOW sin autenticación
- Buffer almacena datos en RAM (no persistente)

## 📚 Funciones Principales

```cpp
void setupLoRa()              // Inicializa receptor LoRa
void setupESPNOW()            // Configura ESP-NOW
void updateOLED()             // Actualiza pantalla

void onDataRecv(...)          // Callback recepción LoRa
void onDataSent(...)          // Callback envío ESP-NOW

void addToBuffer(String msg)  // Almacena en buffer
void sendFromBuffer()         // Envía desde buffer
```

## 🔄 Ciclo Operativo

```
SETUP (2 segundos)
│
LOOP
├─ Escuchar LoRa constantemente
├─ Si recibe:
│  ├─ Validar JSON
│  ├─ Agregar a buffer
│  ├─ Enviar vía ESP-NOW
│  └─ Actualizar OLED
├─ Si buffer lleno:
│  └─ Descartar mensajes antiguos
└─ Repetir
```

## ⚡ Consumo de Energía

```
OLED: ~50 mA
LoRa RX: ~50 mA
ESP-NOW RX: ~80 mA
Lógica: ~20 mA
─────────────────
Promedio: ~200 mA (escucha)
Pico (transmisión): ~500 mA
```

**Autonomía (2500 mAh):** ~12.5 horas en standby

## 📖 Documentación Relacionada

- [README.md](../README.md) - Arquitectura general
- [Trasmisor/README.md](../Trasmisor/README.md) - Especificaciones Trasmisor
- [Lilygo-P/README.md](../Lilygo-P/README.md) - Configuración Gateway
- [TECHNICAL_SPECS.md](../TECHNICAL_SPECS.md) - Especificaciones técnicas

---

**Última actualización:** 16 de Mayo de 2026
