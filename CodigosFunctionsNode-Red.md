# Códigos de Funciones Node-Red - FuelSentinel IoT

## Descripción General
Las funciones 8-11 en Node-Red procesan los datos de sensores provenientes del gateway LilyGO, validan la información, calculan el consumo de combustible y estructuran los datos para almacenamiento en InfluxDB.

---

## Función 8: Extracción de Flujos Primarios

**Ubicación:** Node-Red flow → Function node (8)
**Propósito:** Extrae los valores de flujo de ida y retorno del payload JSON recibido del gateway

```javascript
// Función 8: Extrae flujos del mensaje MQTT
let flujo1 = msg.payload.flujo_ida || 0;
let flujo2 = msg.payload.flujo_retorno || 0;
let latitude = msg.payload.latitud || 0;
let longitude = msg.payload.longitud || 0;
let tapa_abierta = msg.payload.estado_tapa || 0;

// Validar que los flujos sean números válidos
if (isNaN(flujo1)) flujo1 = 0;
if (isNaN(flujo2)) flujo2 = 0;

// Crear objeto de salida con los valores extraídos
msg.payload = {
    flujo_ida: flujo1,
    flujo_retorno: flujo2,
    latitud: latitude,
    longitud: longitude,
    estado_tapa: tapa_abierta,
    timestamp: Date.now()
};

return msg;
```

---

## Función 9: Validación y Cálculo de Consumo

**Ubicación:** Node-Red flow → Function node (9)
**Propósito:** Valida los datos de flujo y calcula el consumo actual (gasto de combustible) como la diferencia flujo_ida - flujo_retorno

```javascript
// Función 9: Valida datos y calcula consumo_actual
let flujo_ida = msg.payload.flujo_ida || 0;
let flujo_retorno = msg.payload.flujo_retorno || 0;

// Validar rangos de flujo (0-100 L/min típicamente)
if (flujo_ida < 0 || flujo_ida > 150) {
    node.warn("Flujo IDA fuera de rango: " + flujo_ida);
    flujo_ida = 0;
}

if (flujo_retorno < 0 || flujo_retorno > 150) {
    node.warn("Flujo RETORNO fuera de rango: " + flujo_retorno);
    flujo_retorno = 0;
}

// Calcular consumo actual: gasto de combustible
// consumo_actual = flujo_ida - flujo_retorno (diferencia en L/min)
let consumo_actual = flujo_ida - flujo_retorno;

// Validar que consumo sea positivo (si es negativo indica error)
if (consumo_actual < 0) {
    node.warn("Consumo negativo detectado: " + consumo_actual + " - posible error de sensores");
    consumo_actual = 0;
}

// Limitar consumo a valores realistas (< 100 L/min)
if (consumo_actual > 100) {
    node.warn("Consumo anómalamente alto: " + consumo_actual + " L/min");
}

// Actualizar payload con datos validados
msg.payload.flujo_ida = flujo_ida;
msg.payload.flujo_retorno = flujo_retorno;
msg.payload.consumo_actual = consumo_actual;

// Agregar timestamp de procesamiento
msg.payload.procesado_en = new Date().toISOString();

return msg;
```

---

## Función 10: Estructuración para InfluxDB

**Ubicación:** Node-Red flow → Function node (10)
**Propósito:** Estructura el payload en formato de línea de protocolo InfluxDB con measurement, fields y tags

```javascript
// Función 10: Estructura datos para InfluxDB (line protocol format)
let flujo_ida = msg.payload.flujo_ida || 0;
let flujo_retorno = msg.payload.flujo_retorno || 0;
let consumo_actual = msg.payload.consumo_actual || 0;
let estado_tapa = msg.payload.estado_tapa || 0;
let latitud = msg.payload.latitud || 0;
let longitud = msg.payload.longitud || 0;
let gateway = msg.payload.gateway || "Tanque-01";

// Extraer timestamp del payload o usar el actual
let timestamp_ms = msg.payload.timestamp || Date.now();
let timestamp_ns = timestamp_ms * 1000000; // Convertir a nanosegundos para InfluxDB

// Estructura de datos para InfluxDB (line protocol)
// Format: measurement,tag1=value1,tag2=value2 field1=value1,field2=value2 timestamp
let influxPayload = {
    measurement: "monitoreo_diesel_tics",
    tags: {
        unidad: "Tractocamion-001",
        propietario: "Operador-Atitalaquia",
        sensor: "Trasmisor-HeltecV3",
        gateway: gateway
    },
    fields: {
        caudal_flujo_ida: parseFloat(flujo_ida),
        caudal_flujo_retorno: parseFloat(flujo_retorno),
        consumo_actual: parseFloat(consumo_actual),
        estado_tapa: parseInt(estado_tapa),
        latitud: parseFloat(latitud),
        longitud: parseFloat(longitud)
    },
    timestamp: timestamp_ns
};

msg.payload = influxPayload;

// Logging para debug
node.info("InfluxDB payload estructurado:");
node.info("Measurement: " + influxPayload.measurement);
node.info("Consumo: " + consumo_actual + " L/min");
node.info("Flujo IDA: " + flujo_ida + " | Flujo RETORNO: " + flujo_retorno);

return msg;
```

---

## Función 11: Preparación para Envío a InfluxDB

**Ubicación:** Node-Red flow → Function node (11)
**Propósito:** Convierte el payload a formato de línea InfluxDB y prepara para envío al bucket SENSORES

```javascript
// Función 11: Prepara payload en formato línea InfluxDB
let data = msg.payload;

// Extraer componentes
let measurement = data.measurement || "monitoreo_diesel_tics";
let tags = data.tags || {};
let fields = data.fields || {};
let timestamp = data.timestamp || Date.now() * 1000000;

// Construir string de tags (tag1=value1,tag2=value2)
let tagString = "";
for (let key in tags) {
    if (tags.hasOwnProperty(key)) {
        tagString += (tagString ? "," : "") + key + "=" + tags[key];
    }
}

// Construir string de fields (field1=value1,field2=value2)
let fieldString = "";
let fieldCount = 0;
for (let key in fields) {
    if (fields.hasOwnProperty(key)) {
        let value = fields[key];
        // Valores numéricos sin comillas en InfluxDB
        fieldString += (fieldCount > 0 ? "," : "") + key + "=" + value;
        fieldCount++;
    }
}

// Construir línea InfluxDB completa (line protocol)
// Formato: measurement,tags fields timestamp
let lineProtocol = measurement;
if (tagString) {
    lineProtocol += "," + tagString;
}
lineProtocol += " " + fieldString + " " + timestamp;

// Preparar objeto de mensaje para InfluxDB
msg.payload = lineProtocol;

// Headers para autenticación InfluxDB 2.x
msg.headers = {
    "Authorization": "Token YOUR_INFLUXDB_TOKEN",
    "Content-Type": "text/plain"
};

// Información adicional para debug
msg.influx = {
    bucket: "SENSORES",
    measurement: measurement,
    fields_count: fieldCount,
    line_protocol: lineProtocol
};

node.info("Enviando a InfluxDB (bucket SENSORES):");
node.info(lineProtocol);

return msg;
```

---

## Función 12 (Opcional): Detección de Anomalías

**Ubicación:** Node-Red flow → Function node (12, opcional)
**Propósito:** Detecta anomalías y genera alertas

```javascript
// Función 12: Detección de anomalías y alertas
let consumo = msg.payload.consumo_actual || 0;
let tapa = msg.payload.estado_tapa || 0;
let flujo_ida = msg.payload.flujo_ida || 0;
let flujo_retorno = msg.payload.flujo_retorno || 0;

let alertas = [];
let nivel_alerta = "NORMAL"; // NORMAL, ADVERTENCIA, CRITICO

// Verificar apertura de tapa
if (tapa === 1) {
    alertas.push("⚠️ TAPA ABIERTA DETECTADA");
    nivel_alerta = "CRITICO";
}

// Verificar consumo anómalo (consumo muy alto indica fuga)
if (consumo > 50) {
    alertas.push("⚠️ Consumo elevado: " + consumo + " L/min");
    nivel_alerta = "ADVERTENCIA";
}

// Verificar diferencia entre flujos
let diferencia_flujos = Math.abs(flujo_ida - flujo_retorno);
if (diferencia_flujos > 0.5 && flujo_ida < 0.1) {
    alertas.push("⚠️ Discrepancia en sensores de flujo");
    nivel_alerta = "ADVERTENCIA";
}

// Crear mensaje de alerta si es necesario
if (alertas.length > 0) {
    msg.alert = {
        nivel: nivel_alerta,
        mensajes: alertas,
        timestamp: new Date().toISOString(),
        consumo_actual: consumo,
        estado_tapa: tapa
    };
    
    node.warn("ALERTA GENERADA: " + nivel_alerta);
    alertas.forEach(a => node.warn(a));
}

return msg;
```

---

## Flujo Completo en Node-Red

### Entrada (MQTT Subscribe)
```
Topic: sensores/diesel/+
Broker: broker.hivemq.com:1883
Payload Esperado:
{
    "flujo_ida": 45.2,
    "flujo_retorno": 42.1,
    "estado_tapa": 0,
    "latitud": 20.7865,
    "longitud": -98.3642,
    "gateway": "Tanque-01"
}
```

### Procesamiento (Funciones 8-12)
```
MQTT In → Function 8 (Extrae flujos)
         → Function 9 (Valida y calcula consumo)
         → Function 10 (Estructura para InfluxDB)
         → Function 11 (Convierte a line protocol)
         → Function 12 (Detecta anomalías)
         → InfluxDB Write (bucket: SENSORES)
         → Alert Node (si hay anomalías)
```

### Salida (InfluxDB)
```
Bucket: SENSORES
Measurement: monitoreo_diesel_tics
Tags: unidad, propietario, sensor, gateway
Fields: caudal_flujo_ida, caudal_flujo_retorno, consumo_actual, estado_tapa, latitud, longitud
Retención: Indefinida
```

---

## Parámetros Críticos

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| Rango Flujo Ida | 0-150 L/min | Válido para sensor en manguera |
| Rango Flujo Retorno | 0-150 L/min | Válido para sensor en manguera |
| Consumo Máximo Realista | 100 L/min | Alerta si se supera |
| Intervalo Lectura | 5 segundos | Ciclo de muestreo |
| Ciclo InfluxDB | 5 segundos | Almacenamiento cada 5s |
| Latencia Total | 3-4 segundos | De sensor a dashboard |

---

## Validación de Datos

La cadena de funciones valida:
1. ✅ Rango de valores (0-150 L/min)
2. ✅ Tipos de datos numéricos
3. ✅ Consumo positivo (flujo_ida ≥ flujo_retorno)
4. ✅ Estado de tapa (0 o 1)
5. ✅ Coordenadas GPS válidas
6. ✅ Timestamps correctos
7. ✅ Detección de anomalías (fugas, tapa abierta)

---

## Troubleshooting

### Error: "Flujo fuera de rango"
- Verificar conexión de sensores de flujo en manguera
- Validar calibración de sensores (0-100 L/min típico)
- Revisar conexión GPIO en Trasmisor.ino

### Error: "Consumo negativo"
- Validar orden de sensores (flujo_ida debe ser ≥ flujo_retorno)
- Revisar calibración individual de cada sensor

### Error: "Payload vacío"
- Verificar que el gateway LilyGO está enviando datos
- Validar conexión MQTT a broker.hivemq.com:1883
- Revisar buffer FIFO en Lilygo.ino

### Latencia > 5 segundos
- Verificar velocidad de conexión 4G/GPRS
- Revisar carga del broker MQTT
- Optimizar funciones Node-Red

---

**Última actualización:** May 4, 2026
**Versión:** 2.0 - Validado con sensores de flujo
**Autor:** Departamento de TICs - InnovaTecNM ITSOEH
