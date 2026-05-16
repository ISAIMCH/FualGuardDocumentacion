# Trasmisor - ESP32 con Sensores

## 📡 Descripción

Unidad de transmisión equipada con módulo LoRa SX1262 que recopila datos de sensores y los transmite a través de radio frecuencia.

## 🎯 Función Principal

Leer sensores cada segundo y transmitir datos vía LoRa cada 5 segundos hacia el receptor/gateway.

## 📦 Contenido

```
Trasmisor/
├── Trasmisor.ino          ← Código principal
└── README.md              ← Este archivo
```

## 🔧 Hardware

| Componente | Especificación | Propósito |
|-----------|----------------|----------|
| Microcontrolador | TTGO LoRa32 v2.1 | Procesamiento |
| Módulo LoRa | SX1262 | Comunicación RF |
| GPS | NEO-6M/M8N | Geolocalización |
| Caudalímetros | 2x Digital | Flujo combustible |
| Sensor Reed | GPIO 6 | Detector tapa |
| OLED | SSD1306 128x64 | Visualización |

## 🚀 Instalación Rápida

```bash
# 1. Descargar Trasmisor.ino
# 2. Abrir en Arduino IDE
# 3. Herramientas → Placa → ESP32 Dev Module
# 4. Cargar código (Ctrl + U)
# 5. Abrir Serial Monitor (115200 baud)
```

## 📊 Flujo de Datos

```
Lectura Sensores (100ms)
        ↓
Procesamiento (cada 1s)
        ↓
Cálculo Métricas (cada 5s)
        ↓
Serialización JSON
        ↓
Transmisión LoRa
        ↓
Esperar 5 segundos
        ↓
[Repetir]
```

## 📤 Datos Transmitidos

```json
{
  "id": "TRANSMITTER_001",
  "timestamp": 1234567890,
  "flujo1_lps": 0.45,
  "flujo2_lps": 0.38,
  "flujo_total": 0.83,
  "alerta_tapa": 1,
  "lat": 20.028306,
  "lng": -99.225007,
  "rssi_lora": -95,
  "snr": -8
}
```

## 🔌 Pines GPIO

| Función | Pin | Voltaje |
|---------|-----|---------|
| FLUJO1 | 4 | 3.3V |
| FLUJO2 | 5 | 3.3V |
| REED | 6 | 3.3V |
| GPS_TX | 47 | 3.3V |
| GPS_RX | 48 | 3.3V |
| OLED_SDA | 17 | 3.3V |
| OLED_SCL | 18 | 3.3V |
| LoRa_SCK | 9 | 3.3V |
| LoRa_MOSI | 10 | 3.3V |
| LoRa_MISO | 11 | 3.3V |
| LoRa_CS | 8 | 3.3V |

## 📝 Configuración

Editar en código líneas 10-20:

```cpp
// Banda LoRa
#define LORA_FREQ 915.0        // MHz (ISM)

// Spreading Factor
#define LORA_SF 9              // Balance alcance/velocidad

// Calibración caudalímetro
#define PPL_FLUJO1 5880        // Pulsos por litro
#define PPL_FLUJO2 5880        // Pulsos por litro
```

## 🧪 Testing

### Serial Monitor (115200 baud)

```
[SETUP] Inicializando Trasmisor...
[OLED] Pantalla activada
[LORA] Módulo SX1262 OK
[GPS] Módulo NEO-6M iniciado
[SENSORES] Contadores calibrados

[GPS] Buscando satélites...
[GPS] 12 satélites adquiridos
[GPS] Lat: 20.028306 Lon: -99.225007

[SENSORES] Flujo1: 0.45 L/s
[SENSORES] Flujo2: 0.38 L/s
[SENSORES] Tapa: CERRADA

[LORA] Transmitiendo 64 bytes...
[LORA] TX completado | RSSI: -95 dBm
[OLED] Actualizando pantalla
```

### Verificación Manual

1. **Flujo:**
   - Verter combustible en tanque
   - Observar incremento en serial

2. **GPS:**
   - Esperar 30 segundos
   - Verificar satélites > 4
   - Coordenadas razonables

3. **Tapa:**
   - Abrir/cerrar tapa
   - Observar cambio en serial

## 📈 Métricas

| Métrica | Valor Típico |
|---------|-------------|
| Consumo promedio | 450 mA |
| Consumo pico (TX) | 600 mA |
| Tiempo TX | 2-3 segundos |
| Intervalo transmisión | 5 segundos |
| Frecuencia LoRa | 915 MHz |
| Spreading Factor | 9 |
| Potencia TX | 10 dBm |

## 🆘 Troubleshooting

### "LoRa no sincroniza"
- [ ] Antena LoRa conectada
- [ ] Voltaje 3.3V en módulo
- [ ] Pines SPI correctos (SCK/MOSI/MISO/CS)
- [ ] No hay conflicto con I2C

### "GPS sin satélites"
- [ ] Antena GPS apuntando al cielo
- [ ] Posición con cielo despejado
- [ ] Esperar 60 segundos en cold start
- [ ] Revisar voltaje 3.3V

### "Caudalímetros sin pulsos"
- [ ] Conexión GPIO correcta
- [ ] Flujo real en sensor
- [ ] Calibrar PPL
- [ ] Comprobar GPIO interrupts habilitadas

## 📚 Código Principal

Funciones principales en `Trasmisor.ino`:

```cpp
void setupLoRa()           // Inicializa LoRa SX1262
void setupGPS()            // Inicializa módulo GPS
void readSensors()         // Lee caudalímetros y Reed
void transmitData()        // Envía JSON vía LoRa
void updateOLED()          // Actualiza pantalla
void onDataReceived()      // Callback de recepción (unused)
```

## 🔄 Ciclo Operativo

```
1. SETUP (3 segundos)
   └─ Inicializar todos los periféricos
   
2. LOOP
   ├─ Lectura sensores (cada 100ms)
   ├─ Actualizar pantalla (cada 1s)
   ├─ Transmitir LoRa (cada 5s)
   └─ Buscar GPS continuamente
```

## ⚡ Consumo de Energía

### Por Componente

```
OLED: ~50 mA
GPS: ~80 mA
LoRa (RX): ~50 mA
LoRa (TX): ~500 mA
Lógica: ~20 mA
─────────────────
Promedio: ~450 mA
```

### Autonomía (Batería 2500 mAh)

```
2500 mAh ÷ 450 mA ≈ 5.5 horas
```

## 📖 Documentación Relacionada

- [README.md](../README.md) - Visión general del proyecto
- [SETUP.md](../SETUP.md) - Instalación y configuración
- [TECHNICAL_SPECS.md](../TECHNICAL_SPECS.md) - Especificaciones detalladas

## 🔗 Referencias

- [RadioLib LoRa Documentation](https://jgromes.github.io/RadioLib/modules/lora/)
- [TinyGPSPlus Library](https://github.com/mikalhart/TinyGPSPlus)
- [U8g2 OLED Library](https://github.com/olikraus/u8g2)

---

**Última actualización:** 16 de Mayo de 2026
