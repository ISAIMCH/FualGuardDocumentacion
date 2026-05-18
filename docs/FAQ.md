# Preguntas Frecuentes (FAQ)

## 🚀 Inicio Rápido

### ¿Por dónde empiezo?

1. Lee [QUICKSTART.md](QUICKSTART.md) (5 minutos)
2. Sigue [SETUP.md](SETUP.md) para instalación
3. Consulta [README.md](README.md) para detalles técnicos

### ¿Necesito comprar hardware específico?

**Sí**, necesitas:
- 2x TTGO LoRa32 v2.1 (~$25 c/u)
- 1x TTGO T-A7 SIM7070 (~$40)
- 1 SIM con datos activa
- Sensores (caudalímetros, GPS, etc.)

**Total:** ~$200 USD (mínimo)

### ¿Funciona con otras placas?

Teóricamente sí, pero requiere adaptar:
- Definiciones de pines GPIO
- Librerías compatibles (RadioLib es universal)
- Test completo antes de usar

**Placas compatibles:**
- TTGO LoRa32 v2.0/v2.1 ✅
- Heltec WiFi LoRa 32 ✅
- Arduino MKR WAN 1300 (parcial)

---

## 🔧 Instalación

### ¿Qué versión de Arduino IDE necesito?

Mínimo: **1.8.13**
Recomendado: **2.0.x+**

[Descargar aquí](https://www.arduino.cc/en/software)

### ¿Cuáles son las librerías requeridas?

Ver [SETUP.md - Paso 5](SETUP.md#paso-5-instalar-librerías)

Resumen rápido:
```
RadioLib, TinyGSM, PubSubClient, 
ArduinoJson, U8g2lib, TinyGPSPlus, Time
```

### ¿Por qué no me aparecen los puertos?

**Windows:**
- Instalar driver CH340
- Reiniciar IDE
- Revisar Device Manager

**Linux:**
```bash
sudo usermod -a -G dialout $USER
# Logout y login
```

**Mac:**
- Instalar [driver Mac CH340](http://www.wch.cn/download/ch341ser_mac_zip.zip)
- Reiniciar

### ¿Se puede usar en Raspberry Pi?

No directamente. Opciones:
1. Usar Arduino IDE en Raspberry + cable USB
2. Usar Arduino CLI para compilación remota
3. Usar Linux con herramientas nativas

---

## 🛜 Conectividad

### ¿Cuál es el alcance LoRa?

**Típico:** 500 metros (línea de vista)
**Máximo:** 10 km (condiciones ideales)
**Mínimo:** 50 m (urbano denso)

Depende de:
- Spreading Factor (SF)
- Potencia de transmisión
- Obstáculos y reflexión
- Antenas

### ¿Necesito WiFi o internet?

**No es requerido para funcionamiento básico:**
- LoRa funciona sin internet
- Buffer local guarda datos
- Sincronización offline-first

**Sí es requerido para:**
- Dashboard web en tiempo real
- Grafana
- MQTT cloud

### ¿Puedo usar otro broker MQTT?

**Sí**, cambiar en código:

```cpp
// Lilygo-P.ino línea 17
const char* broker = "tu-broker.com";
const char* topicPublish = "tu/topic";

// Agregar autenticación si es necesario:
mqtt.setCredentials("usuario", "contraseña");
```

**Opciones:**
- HiveMQ Cloud (recomendado)
- Mosquitto (local)
- AWS IoT Core
- Azure IoT Hub

### ¿Qué significan RSSI y SNR?

**RSSI (Recibida Signal Strength Indicator):**
- Rango: -30 a -120 dBm
- Más cercano a 0 = mejor
- < -110 = muy débil

**SNR (Signal to Noise Ratio):**
- Rango: -20 a +10 dB
- Mayor número = mejor
- < 0 = señal muy débil

---

## 🗺️ GPS

### ¿Por qué tarda en fijar GPS?

**Primera vez (Cold Start):**
- ~30-60 segundos
- Descarga almanaque orbital

**Arranques posteriores (Warm Start):**
- ~10-20 segundos
- Usa datos en caché

**Soluciones:**
```cpp
// Aumentar tiempo de búsqueda en código
#define GPS_TIMEOUT 60000  // 60 segundos

// Posicionar antena verticalmente
// Tener cielo despejado
// Acercar a ventana
```

### ¿Qué precisión tiene GPS?

Típicamente: **±5-10 metros**

Factores que afectan:
- Número de satélites (mínimo 4)
- Cobertura cielo (evitar túneles)
- Multipath urbano
- Reloj del receptor

### ¿Puedo usar GNSS (no solo GPS)?

Sí, módulos como NEO-M8N soportan:
- GPS (USA)
- GLONASS (Rusia)
- Galileo (Europa)
- BeiDou (China)

Mejor precisión y fijación rápida.

---

## ⚡ Sensores

### ¿Cómo calibro los caudalímetros?

```cpp
// Método volumétrico:
// 1. Usar contenedor de volumen conocido (1 litro)
// 2. Llenar y contar pulsos
// 3. PPL = Pulsos / Litro
// 4. Actualizar:

#define PPL_FLUJO1 5880
#define PPL_FLUJO2 5880
```

### ¿Qué precisión tienen los sensores?

| Sensor | Precisión | Error |
|--------|-----------|-------|
| Caudalímetro | ±2% | ±0.02 L/s |
| GPS | ±10m | ±10 metros |
| Sensor Reed | Binario | ±0 (on/off) |
| OLED | Visual | N/A |

### ¿Puedo agregar más sensores?

**Sí**, opciones:

```cpp
// Temperatura (sensor DS18B20)
#define TEMP_PIN 23

// Presión (sensor BMP280)
#define PRESSURE_SDA 21
#define PRESSURE_SCL 22

// Acelerómetro (MPU6050)
#define ACCEL_SDA 21
#define ACCEL_SCL 22
```

Ver [TECHNICAL_SPECS.md](TECHNICAL_SPECS.md) para integración.

---

## 💾 Datos y Almacenamiento

### ¿Dónde se almacenan los datos?

**Trasmisor:** RAM (volátil)
**Gateway:** Buffer circular (10 mensajes máx)
**Nube:** HiveMQ (indefinido según plan)

### ¿Puedo guardar datos localmente?

Sí, opciones:

```cpp
// 1. EEPROM (2 KB disponibles)
EEPROM.write(address, value);

// 2. SPIFFS (1 MB filesystem)
#include <SPIFFS.h>
SPIFFS.begin(true);

// 3. microSD card
#include <SD.h>
```

### ¿Qué pasa si se corta internet?

El sistema:
1. Buffer local almacena datos (~5 minutos)
2. Reintentos automáticos cada 30s
3. Sin pérdida de datos
4. Se sincroniza cuando regresa conexión

---

## 🐛 Problemas Comunes

### Trasmisor no transmite nada

**Checklist:**
- [ ] LoRa módulo conectado?
- [ ] Antena asegurada?
- [ ] Alimentación 3.3V?
- [ ] Serial monitor muestra errores?
- [ ] Pines GPIO correctos?

**Debug:**
```cpp
// En serial monitor:
[LORA] Error inicializando: código X
// Buscar: radio.begin() retorna != RADIOLIB_ERR_NONE
```

### Receptor no recibe

- [ ] Trasmisor transmite? (verificar LED)
- [ ] Mismo Spreading Factor (SF9)?
- [ ] Frecuencia 915 MHz?
- [ ] Distancia < 500m?
- [ ] Antenas conectadas?

### Gateway no envía datos MQTT

```bash
# Test manual:
mosquitto_sub -h broker.hivemq.com \
  -t "itics/mgti/isai/sensor"

# Si no recibe:
# 1. Verificar SIM conectada
# 2. Revisar cobertura celular
# 3. Validar APN
# 4. Revisar logs UART
```

### Dashboard no muestra datos

- [ ] localStorage no limpio? → F12 → Aplicación → Borrar
- [ ] MQTT recibe datos? → Test con mosquitto_sub
- [ ] CORS habilitado en servidor?
- [ ] WebSocket abierto? (F12 → Red)

---

## 📊 Monitoreo

### ¿Cómo veo los datos en tiempo real?

**Opción 1: Monitor serie**
```
Arduino IDE → Herramientas → Monitor Serial
Velocidad: 115200 baud
```

**Opción 2: MQTT**
```bash
mosquitto_sub -h broker.hivemq.com \
  -t "itics/mgti/isai/#" -v
```

**Opción 3: Dashboard web**
```
http://localhost:3000
(o IP del servidor)
```

**Opción 4: Grafana**
```
http://34.145.56.208:3000
Dashboard: adghngx
```

### ¿Qué métricas importante debo monitorear?

1. **RSSI/SNR** - Calidad de señal LoRa
2. **Uptime** - Tiempo operativo
3. **Flujo combustible** - L/minuto
4. **GPS satélites** - Ubicación confiable
5. **Batería** - Porcentaje, voltaje
6. **Errores conexión** - Reconexiones

---

## 🔒 Seguridad

### ¿Es seguro enviar datos a la nube?

**Actual:** No totalmente
- MQTT sin TLS
- Sin autenticación
- Credenciales en código

**Mejoras necesarias:**
```cpp
// Usar TLS:
const char* broker_tls = "broker.hivemq.cloud";
const int port_tls = 8883;
mqtt.setServer(broker_tls, port_tls);

// Agregar credenciales:
mqtt.setCredentials("usuario", "contraseña");

// Usar variables de entorno:
const char* USER = getenv("MQTT_USER");
```

### ¿Se puede interceptar LoRa?

**Teóricamente sí:**
- LoRa no cifrado
- Alcance corto (protección parcial)
- Tráfico no identificable (ventaja)

**Solución:** Encripción de datos
```cpp
#include <mbedtls/aes.h>
// Implementar AES-128
```

### ¿Qué pasa con la privacidad de ubicación?

**Consideraciones:**
- GPS es público en el sistema
- Requiere autorización legal de terceros
- Cumplir GDPR si está en EU
- Informar usuarios sobre rastreo

---

## 💰 Costos Operativos

### ¿Cuánto cuesta operar?

| Componente | Costo Mensual |
|-----------|---------------|
| SIM datos | $5-20 USD |
| HiveMQ Cloud | $0-99 USD |
| Servidor Grafana | $0-50 USD |
| Electricidad | $2-5 USD |
| **Total** | **$7-174 USD** |

### ¿Puedo reducir costos?

**Sí:**
1. Usar Mosquitto local (free)
2. Usar Grafana open-source (free)
3. Plan SIM básico
4. Optimizar transmisión LoRa

---

## 📚 Dónde obtener ayuda

### Documentación

- [README.md](README.md) - Visión general
- [QUICKSTART.md](QUICKSTART.md) - Inicio rápido
- [SETUP.md](SETUP.md) - Instalación
- [TECHNICAL_SPECS.md](TECHNICAL_SPECS.md) - Especificaciones

### Comunidad

- GitHub Issues: Reportar bugs
- GitHub Discussions: Hacer preguntas
- Discord: Chat en tiempo real
- Email: info@fuelguard.dev

### Recursos Externos

- [Arduino Reference](https://www.arduino.cc/reference/)
- [RadioLib Docs](https://jgromes.github.io/RadioLib/)
- [Espressif Docs](https://docs.espressif.com/)
- [MQTT Specification](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/)

---

## ✅ Checklist de Solución de Problemas

Ante cualquier problema, verifica:

```
Hardware:
[ ] Todos los cables conectados
[ ] Alimentación correcta (3.3V o 5V según módulo)
[ ] Antenas aseguradas
[ ] Sin cortocircuitos visibles

Software:
[ ] Arduino IDE v1.8.13+
[ ] Board ESP32 v2.0.x instalado
[ ] Librerías actualizadas
[ ] Código compila sin errores
[ ] Puerto COM correcto

Conectividad:
[ ] LoRa sincronizado
[ ] GPS con satélites
[ ] 4G/MQTT conectado
[ ] No hay interferencia de RF

Datos:
[ ] Serial monitor muestra datos
[ ] MQTT topic recibe mensajes
[ ] Dashboard actualiza
[ ] Grafana grafica
```

---

**¿No encontraste respuesta?** Crear issue: [GitHub Issues](https://github.com/tu-usuario/FuelGuard/issues)

**Última actualización:** 16 de Mayo de 2026
