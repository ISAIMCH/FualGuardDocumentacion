# 📋 DOCUMENTO: TOLERANCIA A FALLOS - SISTEMA FUELGUARD IoT

**Autor:** Isai Montaño Chavez  
**Instituto:** ITSOEH - Mixquiahuala de Juárez, Hidalgo  
**Fecha:** Abril 2026  
**Aplicación:** Sistema de Detección de Extracción de Diésel en Flotas

---

## 1. INTRODUCCIÓN

El sistema FuelGuard implementa múltiples mecanismos de tolerancia a fallos para garantizar confiabilidad en entornos con conectividad intermitente, interferencia electromagnética y fallos parciales de componentes. Este documento especifica los algoritmos de organización de procesos y consenso mediante inundación.

---

## 2. ARQUITECTURA DE PROCESOS

### 2.1 Organización de Grupos de Procesos

El sistema opera con tres grupos de procesos independientes pero coordinados:

```
┌─────────────────────────────────────────────────────────────┐
│                     SISTEMA FUELGUARD                       │
│                  (Tolerancia a Fallos)                      │
└─────────────────────────────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   ┌────▼────┐        ┌───▼────┐        ┌───▼────┐
   │ GRUPO 1 │        │ GRUPO 2│        │ GRUPO 3│
   │ SENSADO │        │PROCESA │        │ COMUNIC│
   └────┬────┘        └───┬────┘        └───┬────┘
        │                  │                  │
        │ TX (5s)          │ Validación      │ MQTT/GSM
        │ ESP-NOW          │ Triple         │ Buffer x10
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                    ┌──────▼──────┐
                    │   STORAGE   │
                    │  (InfluxDB) │
                    └─────────────┘
```

#### **Grupo 1: Sensado (Heltec V3 + Trasmisor)**

| Componente | Responsabilidad | Frecuencia | Tolerancia |
|-----------|-----------------|-----------|-----------|
| Reed Switch | Detección binaria de tapa | Inmediata | Imán pasivo (sin energía) |
| Sensor Flujo Ida | Muestreo volumétrico entrada | 5 segundos | ±2% precisión dual |
| Sensor Flujo Retorno | Muestreo volumétrico salida | 5 segundos | ±2% precisión dual |
| GPS | Geolocalización | 5 segundos | TTFF 1-3 min, fallback a último punto |
| Batería 18650 | Alimentación | Continua | 7+ horas, alerta a 15% |

**Mecanismo de Tolerancia:**
- Si GPS no disponible → usa última ubicación válida
- Si batería baja → reduce frecuencia muestreo a 10s
- Si sensor flujo falla → activa alert "SENSOR_FALLO"

#### **Grupo 2: Procesamiento (Edge Computing - Heltec V3)**

| Componente | Responsabilidad | Validación | Tolerancia |
|-----------|-----------------|-----------|-----------|
| Lectura Sensores | Adquisición de datos | JSON bien formado | Reintentos x3 si error |
| Validación Triple | Verificación consistencia | Ec.1-3 simultáneas | Si falla 1→alerta, si falla 2→descarta |
| Cálculo Consumo | Diferencial Q_ida - Q_ret | |σ < ε| < umbral | Si inconsistencia > 15% → descarta ciclo |
| Filtrado Estadístico | Ruido de vibración motor | Media móvil 5 puntos | Si outlier detectable → promedia |
| Armado JSON | Empaquetamiento datos | Tamaño < 256 bytes | Si > umbral → comprime campos |

**Mecanismo de Tolerancia:**
```
IF validación FALLA:
  contador_fallos++
  IF contador_fallos > 3:
    ENVIAR alerta "VALIDACION_FALLO"
    RESETEAR sensores
  ELSE:
    DESCARTAR ciclo actual
    ESPERAR próximo ciclo (5s)
```

#### **Grupo 3: Comunicación (LilyGO SIM7070 - Gateway)**

| Componente | Responsabilidad | Reintentos | Tolerancia |
|-----------|-----------------|-----------|-----------|
| ESP-NOW RX | Recepción inalámbrica | 1 (buff local) | Descarta si malformado |
| GSM/GPRS | Conexión móvil | Hasta 3x (10s cada) | Fallback a buffer local |
| MQTT Publish | Envío a broker HiveMQ | 1x + buffer | Si falla → almacena x10 registros |
| Buffer Almacenamiento | Persistencia datos | - | Tamaño=10, FIFO antigüedad |
| GPS SIM7070 | Localización alternativa | - | Fallback si receptor falla |

**Mecanismo de Tolerancia:**
```
BUFFER = array[10]
IF MQTT.publish(data) == OK:
  BUFFER.clear()
ELSE:
  IF buffer.size() < MAX:
    buffer.push(data)
  ELSE:
    PERDER dato más antiguo (FIFO)
    buffer.push(data)
```

---

## 3. DIAGRAMA: ORGANIZACIÓN DE GRUPOS DE PROCESOS

```
╔════════════════════════════════════════════════════════════════════════╗
║                   SISTEMA DISTRIBUIDO FUELGUARD                        ║
║                    (3 Grupos de Procesos)                             ║
╚════════════════════════════════════════════════════════════════════════╝

NODO 1: TRASMISOR (Heltec V3)
┌─────────────────────────────────────────────┐
│ GRUPO_SENSADO                               │
│ ┌─────────────────────────────────────────┐ │
│ │ Proceso 1.1: Lectura Reed Switch       │ │
│ │ Proceso 1.2: Lectura Flujo Ida (ISR)   │ │
│ │ Proceso 1.3: Lectura Flujo Retorno     │ │
│ │ Proceso 1.4: Lectura GPS (serial)      │ │
│ └─────────────────────────────────────────┘ │
│ ↓ (cada 5 segundos)                        │
│ GRUPO_PROCESAMIENTO                        │
│ ┌─────────────────────────────────────────┐ │
│ │ Proceso 2.1: Validación Ecuaciones     │ │
│ │ Proceso 2.2: Cálculo Diferencial       │ │
│ │ Proceso 2.3: Filtrado Estadístico      │ │
│ │ Proceso 2.4: Armado JSON               │ │
│ └─────────────────────────────────────────┘ │
│ ↓ (vía ESP-NOW, MAC: 10:06:1C:40:C4:44)    │
│ TRANSMISIÓN: TX → RX a 244 Mbps            │
└─────────────────────────────────────────────┘
                     ↓
                  LATENCIA: 2.8s
                     ↓
NODO 2: RECEPTOR PUENTE (Heltec V3 - Receptor)
┌─────────────────────────────────────────────┐
│ LoRa RX (SF=9, 915 MHz)                    │
│ ↓                                           │
│ Proceso 3.1: Decodificación LoRa           │
│ Proceso 3.2: Validación JSON               │
│ Proceso 3.3: Re-envío a LilyGO (ESP-NOW)  │
│ Proceso 3.4: Display local OLED            │
└─────────────────────────────────────────────┘
                     ↓
                  LATENCIA: +100ms
                     ↓
NODO 3: GATEWAY INTELIGENTE (LilyGO SIM7070)
┌─────────────────────────────────────────────┐
│ GRUPO_COMUNICACIÓN                          │
│ ┌─────────────────────────────────────────┐ │
│ │ Proceso 3.1: ESP-NOW RX                │ │
│ │ Proceso 3.2: Validación GSM/GPRS       │ │
│ │ Proceso 3.3: Enriquecimiento (GPS)     │ │
│ │ Proceso 3.4: MQTT Publish o Buffer     │ │
│ │ Proceso 3.5: Gestión Buffer (FIFO x10) │ │
│ │ Proceso 3.6: Reintento Buffer          │ │
│ └─────────────────────────────────────────┘ │
│ ↓ (vía MQTT con broker HiveMQ)              │
└─────────────────────────────────────────────┘
                     ↓
                  LATENCIA: +1-5s
                     ↓
┌──────────────────────────────────────────────┐
│       CLOUD: MQTT → Node-RED → InfluxDB    │
│       PERSISTENCIA: InfluxDB (series tiempo)│
│       VISUALIZACIÓN: Grafana Dashboard      │
└──────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════

SINCRONIZACIÓN ENTRE GRUPOS:
┌─────────────────────────────────────────────────────────────────┐
│ Evento Crítico: Desconexión GSM                                │
├─────────────────────────────────────────────────────────────────┤
│ T0:  LilyGO pierde señal GSM                                   │
│ T1:  Grupo_Comunicación = ESTADO_BUFFER                        │
│ T2:  Todos los datos posteriores → almacenan en buffer[10]    │
│ T3:  Si buffer.full() AND GSM aún desconectado                │
│      → descartar datos más antiguos (FIFO)                    │
│ T4:  Cuando GSM se recupera                                    │
│      → reenvia buffer automáticamente                          │
│ T5:  Buffer vacío = sincronización completa                   │
└─────────────────────────────────────────────────────────────────┘

```

---

## 4. ALGORITMO DE CONSENSO: INUNDACIÓN (FLOODING)

### 4.1 Descripción Teórica

El algoritmo de **inundación (flood consensus)** es un mecanismo distribuido para alcanzar acuerdo sobre el estado del sistema sin autoridad central. En FuelGuard, se implementa para validar que un evento de extracción es real y debe generar alerta.

**Principios:**
1. Cada nodo propaga mensajes a TODOS sus vecinos
2. Los nodos almacenan mensajes vistos para evitar bucles infinitos
3. Después de k rondas, todos los nodos confiables han recibido el mensaje
4. Si mayoría de nodos confirma condición, se acepta como verdadera

### 4.2 Aplicación en FuelGuard

En nuestro escenario:
- **Nodos:** Trasmisor (sensores), Receptor (puente), Gateway (comunicación)
- **Mensaje de Consenso:** "EXTRACCIÓN DETECTADA = VERDADERO"
- **Condición:** 3 validaciones simultáneas (triple redundancia)

```
CONSENSO REQUERIDO:
┌────────────────────────────────────────────┐
│ Validación 1: Reed Switch = ABIERTO        │ ← Nodo 1 confirma
│ Validación 2: Q_ida - Q_ret > esperado     │ ← Nodo 1 confirma
│ Validación 3: GPS ∉ Z_segura               │ ← Gateway confirma
└────────────────────────────────────────────┘
        Si 2/3 condiciones = OK:
             GENERAR ALERTA
```

---

## 5. DIAGRAMA: ALGORITMO DE CONSENSO POR INUNDACIÓN

```
╔════════════════════════════════════════════════════════════════════╗
║        ALGORITMO DE CONSENSO POR INUNDACIÓN (FLOODING)            ║
║              Para Validación de Eventos de Extracción             ║
╚════════════════════════════════════════════════════════════════════╝

FASE 0: INICIALIZACIÓN (T=0)
═══════════════════════════════════════════════════════════════════

   NODO_A (Trasmisor)         NODO_B (Puente)         NODO_C (Gateway)
   ┌──────────────────┐       ┌──────────────────┐    ┌──────────────────┐
   │ Detecta:         │       │ Estado: IDLE     │    │ Estado: IDLE     │
   │ Reed=ABIERTO     │       │ msgs_visto={}    │    │ msgs_visto={}    │
   │ Q_diff > umbral  │       │                  │    │                  │
   └──────────────────┘       └──────────────────┘    └──────────────────┘
            │
            │ PROPAGA MENSAJE: "EXTRACCIÓN_1"
            ↓

FASE 1: INUNDACIÓN (T=1)
═══════════════════════════════════════════════════════════════════

   NODO_A                      NODO_B                  NODO_C
   (Propositor)               (Relay)                 (Receptor)
   
   ┌─────────────────────────────────────────────────────────────┐
   │ MENSAJE: {                                                  │
   │   id: "EXTR_001",                                          │
   │   timestamp: 1713897600,                                   │
   │   propositor: NODO_A,                                      │
   │   validaciones: [                                          │
   │     {tipo: "REED", valor: ABIERTO, nodo: A},             │
   │     {tipo: "DIFERENCIAL", valor: 15.5, nodo: A},        │
   │     {tipo: "GPS", valor: FUERA_ZONA, nodo: C}           │
   │   ]                                                        │
   │ }                                                           │
   └─────────────────────────────────────────────────────────────┘
        │                    │                   │
        ↓                    ↓                   ↓
    (1.1) Recibe en A    (1.2) Recibe en B   (1.3) Recibe en C
    id_A = "EXTR_001"    id_B = "EXTR_001"   id_C = "EXTR_001"
    visto[A] += msg      visto[B] += msg     visto[C] += msg
        │                    │                   │
        └────────────────────┼───────────────────┘
                             │
                    FASE 2: RETRANSMISIÓN (T=2)
                             │
     Nodo_A                   │              Nodo_C
     reenvía a →             │             ← reenvía a
        B,C             Nodo_B procesa       A,B
        │               y retransmite        │
        └───────────────────→─────────────────┘

FASE 2: VALIDACIÓN LOCAL (T=2)
═══════════════════════════════════════════════════════════════════

   NODO_A:                NODO_B:              NODO_C:
   ┌────────────────┐     ┌────────────────┐   ┌────────────────┐
   │ Valida:        │     │ Valida:        │   │ Valida:        │
   │ Reed=ABIERTO ✓ │     │ Acepta msg de A│   │ GPS coords ✓   │
   │ Q=+15.5 L ✓    │     │ Verifica:      │   │ Verifica triple│
   │ GPS coords ✓   │     │ -Timestamp OK  │   │ -Quórum = 2/3  │
   │                │     │ -Firma válida  │   │ -Consenso=SATA │
   │ Quórum = 2/3 ✓ │     │ -No repetido    │   │                │
   │ CONSENSO=SATA  │     │ Quórum = 2/3 ✓ │   │ CONSENSO=SATA  │
   └────────────────┘     └────────────────┘   └────────────────┘
          ↓                     ↓                    ↓
      resultado:           resultado:          resultado:
     ACEPTA                 ACEPTA             ACEPTA

FASE 3: PROPAGACIÓN DE CONSENSO (T=3)
═══════════════════════════════════════════════════════════════════

   ┌────────────────────────────────────────────────────────────┐
   │              TODOS LOS NODOS ACORDARON:                   │
   │          EVENTO: EXTRACCIÓN_001 = CONFIRMADO             │
   │                                                            │
   │  Acción Resultante:                                       │
   │  ├─ Trasmisor (A):  Registra en log local                 │
   │  ├─ Puente (B):     Retransmite alerta a Gateway          │
   │  └─ Gateway (C):    GENERA ALERTA + Envía MQTT            │
   │                     tópico: itics/mgti/isai/alerta       │
   └────────────────────────────────────────────────────────────┘

PSEUDOCÓDIGO DEL ALGORITMO:
═══════════════════════════════════════════════════════════════════

función FLOODING_CONSENSO(mensaje):
    
    // Fase 0: Inicialización
    msg_id ← hash(mensaje)
    visto[mi_nodo] ← {msg_id}
    contador_confirmaciones ← 0
    
    // Fase 1: Recibir mensaje
    si msg_id ∉ visto:
        visto ← visto ∪ {msg_id}
        
        // Fase 2: Validar localmente
        si VALIDAR_ECUACIONES(mensaje):
            contador_confirmaciones++
        
        // Retransmitir a vecinos
        para cada vecino en VECINOS:
            si vecino ≠ remitente:
                ENVIAR(mensaje, vecino)
    
    // Fase 3: Decidir consenso
    si contador_confirmaciones ≥ QUÓRUM_MÍNIMO (2/3):
        estado ← CONSENSO_ALCANZADO
        GENERAR_ALERTA(mensaje)
        retorna VERDADERO
    sino:
        estado ← EN_ESPERA
        retorna FALSO

PARÁMETROS:
───────────
Ronda (round): 2.8 segundos (latencia triple validación)
Timeout: 10 segundos (máximo espera consenso)
Quórum: 2/3 de nodos (mayoría simple)
Almacenamiento: visto[] = set(msg_id) previene bucles
Máx intentos: 3 rondas de inundación

```

---

## 6. MECANISMO DE TOLERANCIA A FALLOS CON INUNDACIÓN

### 6.1 Escenarios de Fallo Manejados

```
ESCENARIO 1: Nodo Trasmisor falla (sensores desconectados)
─────────────────────────────────────────────────────────────
T0:  Trasmisor detiene envío
T2:  Gateway no recibe datos por 10 segundos
T3:  Gateway registra ESTADO_FALLO_TRASMISOR
T4:  Genera alerta: "PÉRDIDA_SEÑAL_SENSORES"
Resultado: ✓ Sistema alerta caída de sensores (no falso negativo)

ESCENARIO 2: Gateway desconexión GSM (sin internet)
──────────────────────────────────────────────────
T0:  Gateway pierde señal GSM/GPRS
T1:  Trasmisor sigue enviando datos (ESP-NOW OK)
T2:  Gateway activa modo BUFFER
T3:  Almacena hasta 10 registros localmente
T4:  GSM se recupera → reenvía buffer automáticamente
Resultado: ✓ Sin pérdida de datos, consenso pospuesto hasta recovery

ESCENARIO 3: Fallo de validación (error de sensor flujo)
────────────────────────────────────────────────────────
T0:  Sensor flujo lee valor inconsistente (+999 L/min)
T1:  Ecuación de validación detecta outlier
T2:  Descarta ciclo actual, retiene histórico
T3:  Próximo ciclo valida nuevamente
Resultado: ✓ Rechaza falsas detecciones por ruido

ESCENARIO 4: Algoritmo Inundación con timeout
──────────────────────────────────────────────
T0:  Evento de extracción confirmado por 2/3 nodos
T1:  Iniciata inundación (flooding)
T2:  Si Nodo B no responde en 10s → continúa sin B
T3:  Si 2/3 confirman (A+C) → CONSENSO VÁLIDO
T4:  Alerta generada aunque B esté offline
Resultado: ✓ Tolerancia a 1 nodo caído (f=1, n=3)

```

### 6.2 Matriz de Tolerancia a Fallos

| Componente | Fallo Posible | Detección | Recuperación | Pérdida Datos |
|-----------|---------------|-----------|--------------|--------------|
| Reed Switch | No responde | Comparación estado | Reinicio en 5s | NO |
| Flujo Ida | Lectura errática | Validación diferencial | Descarta ciclo | NO |
| Flujo Retorno | Sensor desconectado | Diferencial ∞ | Alerta sensor | SÍ* |
| GPS Trasmisor | TTFF > 3min | Usa último punto válido | Espera 60s | NO (parcial) |
| Batería | Baja (< 15%) | Monitoreo voltage | Reduce frecuencia a 10s | NO |
| ESP-NOW TX | Fallo emisión | Callback status | Reintenta x3 | NO |
| GSM/GPRS | Desconexión | Timeout red | Reintenta x3 + Buffer | NO |
| MQTT Broker | Inaccesible | Fallo publish | Almacena buffer x10 | NO** |
| Buffer Lleno | > 10 registros | Contador índice | Descarta antiguo (FIFO) | SÍ (1 dato) |

**SÍ\* = Si ambos sensores flujo fallan simultáneamente  
**NO** = Los 10 primeros registros se preservan

---

## 7. CONCLUSIONES

**Tolerancia a Fallos Implementada:**
- ✅ Redundancia triple (triple validación)
- ✅ Buffer inteligente con FIFO (10 registros)
- ✅ Algoritmo de inundación para consenso distribuido
- ✅ Reintentos automáticos (red, MQTT, sensores)
- ✅ Detección de anomalías (validación estadística)
- ✅ Fallback a GPS alternativo (modem SIM7070)

**Limitaciones Conocidas:**
- ⚠️ Si fallan 2/3 flujos: no hay consumo válido
- ⚠️ Buffer limitado a 10 registros (máximo 50 segundos sin conexión)
- ⚠️ MQTT reintenta solo 1 vez (mejorable a 3x)
- ⚠️ Timeout red fijo 10s (subóptimo en zonas rurales)

**Recomendación Operacional:**
Para máxima confiabilidad en flotas de carga, considerar:
1. Aumentar buffer a 20 registros
2. Aumentar timeout GSM a 30 segundos
3. Implementar reintentos MQTT x3
4. Agregar GPS redundante (backup u-blox)

---

**Documento:** TOLERANCIA_A_FALLOS.md  
**Versión:** 1.0  
**Última actualización:** Abril 23, 2026  
**Estado:** COMPLETO ✓
