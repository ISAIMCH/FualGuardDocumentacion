# Guía de Instalación Rápida

## 🎯 Start Here!

### Opción 1: Instalación Local (5 minutos)

```bash
# 1. Clonar el repositorio
git clone https://github.com/tu-usuario/FuelGuard.git
cd FuelGuard

# 2. Instalar dependencias de Arduino
# Ir a Arduino IDE → Preferences
# Agregar URL: https://dl.espressif.com/dl/package_esp32_index.json

# 3. Instalar Board Manager
# Tools → Board Manager → Buscar ESP32 → Instalar v2.0.x

# 4. Cargar código
# Abrir Trasmisor.ino → Cargar firmware
```

### Opción 2: Docker (si aplica)

```bash
docker run -p 3000:3000 fuelguard:latest
```

---

## ⚙️ Configuración Inicial

### Paso 1: Editar Credenciales

**Archivo:** `Lilygo-P/Lilygo-P.ino` línea 10-12

```cpp
const char apn[] = "web.iusacellgsm.mx";
const char gprsUser[] = "tu_usuario";
const char gprsPass[] = "tu_contraseña";
```

### Paso 2: Configurar LoRa

**Archivo:** `Trasmisor/Trasmisor.ino` línea 85

```cpp
int state = radio.begin(915.0);  // ISM Band
radio.setOutputPower(10);         // dBm
radio.setSpreadingFactor(9);      // SF (balance rango/velocidad)
```

### Paso 3: Verificar Conexiones

```bash
# Monitor serial a 115200 baud
# Buscar:
# - [GPS] Adquiriendo satélites...
# - [LORA] Módulo OK
# - [MQTT] Conectado
```

---

## 🧪 Testing

### Test 1: Conexión LoRa
```bash
# En Trasmisor serial monitor (debe mostrar):
[LORA] TX: {"flujo1": 0.45, ...}
```

### Test 2: Conectividad MQTT
```bash
mosquitto_sub -h broker.hivemq.com -t "itics/mgti/isai/sensor"
# Debe recibir mensajes JSON cada 5 segundos
```

### Test 3: Dashboard
```bash
http://localhost:3000
# Debe mostrar datos en tiempo real
```

---

## 📱 Troubleshooting Rápido

| Problema | Solución |
|----------|----------|
| No hay GPS | Activar módulo GPS en receiver, esperar 30s |
| LoRa no sincroniza | Revisar SF en ambos = 9, frecuencia = 915MHz |
| MQTT sin conexión | Verificar SIM activa, APN correcto |
| Dashboard en blanco | Borrar localStorage, F5 reload |

---

## 📖 Siguiente Paso

Leer [README.md](README.md) para documentación completa.
