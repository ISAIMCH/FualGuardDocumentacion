# Trasmisor - ESP32 con Sensores

## 📡 Descripción

Unidad de transmisión equipada con módulo LoRa SX1262 que recopila datos de sensores y los transmite a través de radio frecuencia.

## 🎯 Función Principal

Leer sensores cada segundo y transmitir datos vía LoRa cada 5 segundos hacia el receptor/gateway.

## 🔧 Hardware

| Componente | Especificación | Propósito |
|-----------|----------------|----------|
| Microcontrolador | TTGO LoRa32 v2.1 | Procesamiento |
| Módulo LoRa | SX1262 | Comunicación RF |
| GPS | NEO-6M/M8N | Geolocalización |
| Caudalímetros | 2x Digital | Flujo combustible |
| Sensor Reed | GPIO 6 | Detector tapa |
| OLED | SSD1306 128x64 | Visualización |

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

## 🚀 Instalación rápida (Arduino IDE)

1. Abrir `Trasmisor/Trasmisor.ino` en Arduino IDE.
2. Herramientas → Placa → `ESP32 Dev Module`.
3. Cargar firmware (Upload) a la placa TTGO LoRa32.
4. Abrir Serial Monitor a `115200` baudios para depuración.

## 🧪 Testing básico

- Verificar mensajes de inicialización en Serial Monitor.
- Probar flujo, GPS y lectura de tapa (Reed).
- Confirmar transmisión LoRa y valores RSSI/SNR.

## 📚 Referencias

- Código: `Trasmisor/Trasmisor.ino`
- Documentación principal: [README.md](../README.md)

---

**Última actualización:** 16 de Mayo de 2026
