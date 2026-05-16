# Lilygo-P - Gateway 4G/LTE con SIM7070

## 📡 Descripción

Unidad gateway que conecta el sistema a internet mediante módulo 4G LTE (SIM7070) y publica datos a MQTT.

## 🎯 Función Principal

Recibir datos de ESP-NOW → conectar a internet → publicar en broker MQTT HiveMQ.

## 📦 Contenido

```
Lilygo-P/
├── Lilygo-P.ino           ← Código principal
└── README.md              ← Este archivo
```

## 🔧 Hardware

| Componente | Especificación |
|-----------|----------------|
| Microcontrolador | TTGO T-A7 Dual Core |
| Módulo Celular | SIM7070 (4G LTE) |
| GPS | Integrado en SIM7070 |
| Batería | XT60 conector |

## 📍 Ubicación Física

```
El Gateway debe estar:
✓ En lugar elevado (mejor cobertura)
✓ Con cielo despejado para GPS
✓ Alejado del trasmisor (evitar interferencia)
✓ Conectado a fuente de poder permanente
✓ Antenna SMA hacia el cielo
```

## 🚀 Instalación

```bash
# 1. Insertar SIM activa con datos
# 2. Conectar antenna 4G (SMA)
# 3. Abrir Lilygo-P.ino en Arduino IDE
# 4. Configurar APN (línea 10-12)
# 5. Cargar firmware
# 6. Verificar conexión en serial monitor
```

## 🔌 Configuración APN

**IMPORTANTE:** Editar líneas 10-12

```cpp
// Iusacell (Mexnet - México)
const char apn[] = "web.iusacellgsm.mx";
const char gprsUser[] = "iusacellgsm";
const char gprsPass[] = "iusacellgsm";

// ó cambiar según tu proveedor:
// Telcel: iusacellgsm.mx / iusacellgsm / iusacellgsm
// AT&T: mobile.att.com / att / att
// Movistar: movistar.mx / movistar / movistar
```

## 🌐 Configuración MQTT

**Línea 15-16:**

```cpp
const char* broker = "broker.hivemq.com";
const char* topicPublish = "itics/mgti/isai/sensor";

// Optional: Agregar autenticación (v1.1+)
mqtt.setCredentials("usuario", "contraseña");
```

## 🔄 Flujo de Conexión

```
      Receptor
      (ESP-NOW)
         ↓
    Lilygo-P
         │
         ├─→ Red Celular (SIM7070)
         │      ├─ Buscar cobertura
         │      ├─ GPRS Connect
         │      └─ IP asignada
         │
         ├─→ Broker MQTT
         │      ├─ Conexión TCP/IP
         │      ├─ Auth (si aplica)
         │      └─ Conexión OK
         │
         └─→ Publicar
              └─ topic: itics/mgti/isai/sensor
```

## 📥/📤 Datos MQTT

### Entrada (ESP-NOW from Receptor)

```json
{
  "id": "TRANSMITTER_001",
  "flujo1": 0.45,
  "flujo2": 0.38,
  "alerta": 1,
  "lat": 20.0283,
  "lng": -99.2250,
  "ts": 1234567890
}
```

### Salida (MQTT publish)

```
Topic: itics/mgti/isai/sensor
Payload: [JSON anterior]
QoS: 1 (At least once)
Retain: false
```

## 📊 Buffer de Almacenamiento

```cpp
#define MAX_BUFFER 10
String bufferDatos[MAX_BUFFER];
int bufferIndex = 0;

// Funciona como:
// - Almacena up to 10 mensajes
// - Si no hay internet, buffer se llena
// - Al conectar, envía todos
// - Circular: sobrescribe si está lleno
```

## 🧪 Testing

### Serial Monitor (115200 baud)

```
[SETUP] Inicializando Lilygo-P...
[4G] Inicializando modem SIM7070...

[4G] Buscando red...
[4G] Buscando red... (5s)
[4G] Conectado a Iusacell

[GPS] Buscando satélites...
[GPS] Satélites: 12
[GPS] Lat: 20.028306 Lon: -99.225007

[MQTT] Conectando a broker.hivemq.com...
[MQTT] Conectado

[ESP-NOW] Esperando datos del Receptor...
[ESP-NOW] Datos recibidos

[MQTT] Publicando en itics/mgti/isai/sensor
[MQTT] OK

Buffer: 0/10
Uptime: 00:01:23
```

### Verificación Manual MQTT

```bash
# Terminal en otra computadora
mosquitto_sub -h broker.hivemq.com \
  -t "itics/mgti/isai/#" -v

# Debe mostrar:
itics/mgti/isai/sensor {"flujo1": 0.45, ...}
itics/mgti/isai/sensor {"flujo1": 0.44, ...}
[cada 5 segundos]
```

## 🆘 Troubleshooting

### "No hay cobertura celular"
- [ ] SIM insertada correctamente?
- [ ] Antenna SMA conectada?
- [ ] Revisar saldo/plan SIM?
- [ ] Esperar 30 segundos
- [ ] Cambiar ubicación (menos obstáculos)

**Debug:**
```cpp
String ops = modem.getOperators();
Serial.println(ops);  // Ver redes disponibles
```

### "MQTT no conecta"
- [ ] Cobertura celular OK?
- [ ] Credenciales correctas?
- [ ] Broker online?
- [ ] Firewall permitiendo 1883?
- [ ] Topic válido?

**Test:**
```bash
telnet broker.hivemq.com 1883
# Debe conectar
```

### "GPS sin satélites"
- [ ] Antenna SMA bien orientada?
- [ ] Cobertura celular disponible (necesaria para A-GPS)?
- [ ] Esperar 60 segundos en cold start?
- [ ] Cielo despejado?

### "Buffer lleno, datos perdidos"
- [ ] Internet disponible?
- [ ] Broker recibiendo?
- [ ] Revisar publicaciones
- [ ] Aumentar MAX_BUFFER?

## 📈 Métricas

| Métrica | Valor Típico |
|---------|-------------|
| Consumo 4G Inactivo | 100 mA |
| Consumo 4G Conectado | 200 mA |
| Pico de transmisión | 2000 mA |
| Latencia MQTT | 1-5 segundos |
| Tasa de error | < 0.1% |

## ⚡ Consumo de Energía

```
GPS activo: ~100 mA
4G standby: ~100 mA
4G transmitiendo: ~1500 mA
Lógica: ~50 mA
─────────────────────
Promedio: ~250 mA
```

**Nota:** Requiere fuente de poder; no usar batería portátil.

## 📱 Estados del Dispositivo

```cpp
enum Estado {
  INIT,              // Inicialización
  MODEM_INIT,        // Modem iniciando
  SEARCHING_NETWORK, // Buscando red
  CONNECTED_4G,      // Conectado 4G
  ACQUIRING_GPS,     // Buscando GPS
  GPS_READY,         // GPS listo
  MQTT_CONNECTING,   // Conectando MQTT
  MQTT_CONNECTED,    // MQTT listo
  SENDING,           // Enviando datos
  ERROR              // Error
};
```

## 🔐 Seguridad (v1.1+)

**Mejorar autenticación MQTT:**

```cpp
// Agregar usuario/contraseña:
mqtt.setCredentials("usuario_mqtt", "password_mqtt");

// Usar TLS (broker.hivemq.cloud:8883):
#define MQTT_USE_TLS true
wifi.setClient(&tlsClient);
```

## 📚 Comandos AT SIM7070

```cpp
// Algunos comandos útiles:
modem.sendAT("+CGPS=1");           // Activar GPS
modem.getOperators();               // Ver redes
modem.getRSSI();                    // Señal RSSI
modem.getBattVoltage();             // Voltaje batería
modem.getModemInfo();               // Info modem
```

## 📖 Funciones Principales

```cpp
void setupModem()                  // Inicializa modem
void asegurarConexion()            // Reconecta si falla
void leerDatosESPNOW()             // Recibe de Receptor
void procesarJSON()                // Valida datos
void enviarMQTT()                  // Publica a broker
void actualizarBuffer()            // Gestiona almacenamiento
void mostrarStatus()               // Debug info
```

## 🔄 Ciclo Operativo

```
SETUP (5 segundos)
│
LOOP
├─ Verificar conexión 4G
├─ Escuchar ESP-NOW
├─ Si hay datos:
│  ├─ Validar JSON
│  ├─ Enviar a MQTT
│  └─ Si falla, buffer
├─ Si buffer no vacío:
│  ├─ Intentar enviar
│  └─ Vaciar buffer
├─ Actualizar status
└─ Repetir cada 5s
```

## 🌍 Cobertura Global

El SIM7070 soporta:

```
Bandas LTE FDD:
├─ B1, B2, B3, B4, B5
├─ B7, B8, B12, B13
├─ B14, B17, B18, B19, B20
├─ B25, B26, B28, B29, B32
└─ B66 (Americas)

Cat-M1 & NB-IoT también soportados
```

## 📊 Cobertura Iusacell (México)

```
Bandas 4G:
├─ AWS 1700/2100 MHz (B4)
├─ 2.6 GHz (B7)
└─ 700 MHz (B28)

Cobertura: ~95% ciudades, ~80% rural
```

## ⚠️ Limitaciones

1. Requiere SIM con datos activa
2. No funciona sin cobertura celular
3. Buffer limitado a 10 mensajes
4. Latencia variable según red
5. Costo por datos de la SIM

## 📞 Soporte del Proveedor

**Iusacell México:**
- Web: https://www.iusacell.com.mx
- Soporte: 01-800-IUSACELL

**Otros proveedores:**
- Cambiar APN según región
- Contactar al proveedor

## 📖 Documentación Relacionada

- [README.md](../README.md) - Arquitectura general
- [Receptor/README.md](../Receptor/README.md) - Configuración Receptor
- [TECHNICAL_SPECS.md](../TECHNICAL_SPECS.md) - Especificaciones técnicas
- [SETUP.md](../SETUP.md) - Guía de instalación

---

**Última actualización:** 16 de Mayo de 2026
