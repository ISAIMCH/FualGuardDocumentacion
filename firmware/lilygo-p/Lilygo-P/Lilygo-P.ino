#define TINY_GSM_MODEM_SIM7070
#define TINY_GSM_RX_BUFFER 1024 

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <esp_now.h>
#include <WiFi.h> // Se mantiene la librería únicamente porque ESP-NOW la requiere para la capa MAC
#include <ArduinoJson.h>

// ===================== PINES =====================
#define MODEM_TX      27
#define MODEM_RX      26
#define MODEM_PWRKEY  4
#define LED_PIN       12

// ===================== CONFIGURACIÓN RED CELULAR =====================
const char apn[]      = "web.iusacellgsm.mx";
const char gprsUser[] = "iusacellgsm";
const char gprsPass[] = "iusacellgsm";

const char* broker = "broker.hivemq.com";
const char* topicPublish = "itics/mgti/isai/sensor";

// ===================== OBJETOS DE CONEXIÓN =====================
HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient clientGPRS(modem); 
PubSubClient mqtt(clientGPRS);

// ===================== VARIABLES DE CONTROL =====================
String payloadESPNow = "";
volatile bool hayDatoNuevo = false;

enum EstadoModem { ENCIENDO_KEY, APAGO_KEY, ESPERANDO_ESTABILIZACION, MODEM_LISTO };
EstadoModem pasoModem = ENCIENDO_KEY;

#define MAX_BUFFER 10
String bufferDatos[MAX_BUFFER];
int bufferIndex = 0;

unsigned long cronometroModem = 0;
unsigned long ultimoIntentoRed = 0;
const unsigned long intervaloIntentoRed = 15000; 

// ===================== CALLBACK ESP-NOW =====================
void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  char buffer[len + 1];
  memcpy(buffer, incomingData, len);
  buffer[len] = '\0';

  payloadESPNow = String(buffer);
  hayDatoNuevo = true;
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(100); 
  
  Serial.println("\n=== INICIALIZANDO GATEWAY CELULAR FUELGUARD ===");

  pinMode(LED_PIN, OUTPUT);
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, LOW);

  // 1. Configurar radio estrictamente para ESP-NOW (WiFi deshabilitado para evitar conflictos)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); 
  Serial.print("[SISTEMA] Dirección MAC de la LilyGO: ");
  Serial.println(WiFi.macAddress());

  // 2. Arrancar ESP-NOW de forma inmediata (Máxima prioridad de escucha local)
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Error crítico al inicializar el receptor.");
  } else {
    Serial.println("[ESP-NOW] Receptor activo. Escuchando telemetría de la Heltec V3...");
  }
  esp_now_register_recv_cb(onDataRecv);

  // 3. Inicializar puerto serie de comandos AT para el módem
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  // 4. Configurar Broker MQTT sobre la red celular
  mqtt.setServer(broker, 1883);
  mqtt.setKeepAlive(60); 

  cronometroModem = millis();
}

// ===================== MAQUINA DE ESTADOS ASÍNCRONA DEL MÓDEM =====================
void actualizarHardwareModem() {
  unsigned long tiempoActual = millis();

  switch (pasoModem) {
    case ENCIENDO_KEY:
      Serial.println("[MÓDEM] Enviando pulso de encendido a PWRKEY...");
      digitalWrite(MODEM_PWRKEY, HIGH);
      cronometroModem = tiempoActual;
      pasoModem = APAGO_KEY;
      break;

    case APAGO_KEY:
      if (tiempoActual - cronometroModem >= 1500) {
        digitalWrite(MODEM_PWRKEY, LOW);
        Serial.println("[MÓDEM] Pulso finalizado. Esperando respuesta del firmware interno...");
        cronometroModem = tiempoActual;
        pasoModem = ESPERANDO_ESTABILIZACION;
      }
      break;

    case ESPERANDO_ESTABILIZACION:
      if (tiempoActual - cronometroModem >= 4000) {
        Serial.println("[MÓDEM] Validando comunicación mediante comandos AT...");
        if (modem.testAT()) {
          Serial.println("[MÓDEM] >>> HARDWARE SIM7070G OPERATIVO Y RESPONDIENDO OK <<<");
          pasoModem = MODEM_LISTO;
        } else {
          Serial.println("[MÓDEM] [AVISO] Sin respuesta AT. Reintentando secuencia de encendido...");
          pasoModem = ENCIENDO_KEY; 
        }
      }
      break;

    case MODEM_LISTO:
      break;
  }
}

// ===================== GESTOR DE RED CELULAR NO BLOQUEANTE =====================
void asegurarConexion() {
  if (pasoModem != MODEM_LISTO) return;

  unsigned long tiempoActual = millis();
  
  if (tiempoActual - ultimoIntentoRed >= intervaloIntentoRed || ultimoIntentoRed == 0) {
    ultimoIntentoRed = tiempoActual;

    if (!modem.isNetworkConnected()) {
      Serial.println("[CELULAR] Buscando repetidor / torre celular en la zona...");
      if (!modem.waitForNetwork(1000)) { // Timeout de 1 segundo para no congelar el loop
        Serial.println("[CELULAR] Red celular no disponible por el momento.");
        return;
      }
    }

    if (modem.isNetworkConnected() && !modem.isGprsConnected()) {
      Serial.println("[GPRS] Solicitando conexión de datos al APN...");
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        Serial.println("[GPRS] Falló la autenticación o conexión de datos.");
        return;
      } else {
        Serial.println("[GPRS] >>> CONECTADO CORRECTAMENTE A INTERNET CELULAR <<<");
      }
    }
  }
}

// 🔄 Asegurar Servidor MQTT
void asegurarMQTT() {
  if (!modem.isGprsConnected()) return;

  if (!mqtt.connected()) {
    Serial.print("[MQTT] Conectando al broker HiveMQ vía GPRS Celular...");
    String clientId = "GatewayFuelGuard-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println(" ¡CONECTADO EXITOSAMENTE!");
    } else {
      Serial.print(" Falló conexión. Código rc = ");
      Serial.println(mqtt.state());
    }
  }
}

// 📤 Enviar dato o guardar en buffer
void enviarODefinirBuffer(String data) {
  // Mostrar en consola exactamente el JSON que va saliendo hacia Grafana/Node-RED
  Serial.println("\n--------------------------------------------------");
  Serial.println("[MQTT SEND] >>> TRANSMITIENDO AL BROKER HIVEMQ <<<");
  Serial.print("[MQTT DATA] Payload: ");
  Serial.println(data);
  
  if (modem.isGprsConnected() && mqtt.connected() && mqtt.publish(topicPublish, data.c_str())) {
    Serial.println("[MQTT STATUS] ¡Confirmación de recepción recibida con éxito!");
    Serial.println("--------------------------------------------------\n");
  } else {
    Serial.println("[BUFFER] Servidor inalcanzable. Guardando paquete en memoria local...");
    Serial.println("--------------------------------------------------\n");
    if (bufferIndex < MAX_BUFFER) {
      bufferDatos[bufferIndex++] = data;
    } else {
      Serial.println("[BUFFER LLENO] Descartando el registro histórico más antiguo.");
      for (int i = 1; i < MAX_BUFFER; i++) {
        bufferDatos[i - 1] = bufferDatos[i];
      }
      bufferDatos[MAX_BUFFER - 1] = data;
    }
  }
}

// 🔁 Reenviar buffer
void enviarBufferPendiente() {
  if (bufferIndex == 0 || !mqtt.connected()) return;

  Serial.print("[HISTORIAL] Procesando y vaciando ");
  Serial.print(bufferIndex);
  Serial.println(" paquetes retenidos en cola...");
  int enviadosConExito = 0;

  for (int i = 0; i < bufferIndex; i++) {
    if (mqtt.publish(topicPublish, bufferDatos[i].c_str())) {
      enviadosConExito++;
    } else {
      break; 
    }
  }

  if (enviadosConExito > 0) {
    int restantes = bufferIndex - enviadosConExito;
    for (int i = 0; i < restantes; i++) {
      bufferDatos[i] = bufferDatos[i + enviadosConExito];
    }
    bufferIndex = restantes;
    Serial.println("[HISTORIAL] Sincronización de cola completada.");
  }
}

// ===================== LOOP PRINCIPAL (100% INMUNE A CUELGUES) =====================
void loop() {
  // 1. Procesar secuencia física del módem en segundo plano
  actualizarHardwareModem();

  // 2. Monitorear e interconectar a la red de datos celular
  asegurarConexion();

  // 3. Si hay canal GPRS activo, mantener vivo MQTT y enviar pendientes
  if (modem.isGprsConnected()) {
    asegurarMQTT();
    if (mqtt.connected()) {
      mqtt.loop();
      enviarBufferPendiente();
    }
  }

  // 4. Recepción y procesamiento instantáneo desde Heltec por ESP-NOW
  if (hayDatoNuevo) {
    digitalWrite(LED_PIN, HIGH);
    
    // Imprimir en consola el paquete crudo tal y como va llegando por el aire desde la Heltec
    Serial.println("\n==================================================");
    Serial.println("[ESP-NOW RECV] <<< PAQUETE DE DATOS ENTRANTE >>>");
    Serial.print("[ESP-NOW DATA] JSON Crudo: ");
    Serial.println(payloadESPNow);
    Serial.println("==================================================");

    StaticJsonDocument<384> docRecibido;
    DeserializationError error = deserializeJson(docRecibido, payloadESPNow);
    
    if (!error) {
      int f1 = docRecibido["flujo1"];
      int f2 = docRecibido["flujo2"];
      int reed = docRecibido["reed"];
      float latitud_heltec = docRecibido["lat"];
      float longitud_heltec = docRecibido["lng"];

      static unsigned long acumuladoF1 = 0;
      static unsigned long acumuladoF2 = 0;
      
      acumuladoF1 += f1;
      acumuladoF2 += f2;

      String diagnostico = "OPERACION_NORMAL";
      bool esAlertaCritica = false;

      if (f1 > 0 && f2 == 0) {
        diagnostico = "ALERTA_ORDEÑA_RETORNO";
        esAlertaCritica = true;
      }
      else if (reed == 0) {
        if (f1 > 0) {
          diagnostico = "ALERTA_SABOTAJE_TAPA_EN_MOVIMIENTO";
          esAlertaCritica = true;
        } else {
          diagnostico = "ALERTA_APERTURA_SOSPECHOSA";
          esAlertaCritica = true;
        }
      }
      else if (f1 == 0 && f2 == 0) {
        diagnostico = "APAGADO_SEGURO";
      }

      static String ultimoEstadoEnviado = "";
      static unsigned long ultimoEnvioMQTT = 0;
      unsigned long tiempoActual = millis();
      
      unsigned long intervaloTransmision = 300000;

      if (diagnostico == "OPERACION_NORMAL") {
        intervaloTransmision = 60000;  
      } else if (esAlertaCritica) {
        intervaloTransmision = 3000;  
      }

      if (esAlertaCritica || (diagnostico != ultimoEstadoEnviado) || (tiempoActual - ultimoEnvioMQTT >= intervaloTransmision) || ultimoEnvioMQTT == 0) {
        
        StaticJsonDocument<384> docEnvio;
        docEnvio["flujo1"] = acumuladoF1;
        docEnvio["flujo2"] = acumuladoF2;
        docEnvio["lat"] = latitud_heltec;    
        docEnvio["lng"] = longitud_heltec;   
        docEnvio["gateway"] = "Tanque-01";
        docEnvio["status"] = diagnostico;

        char bufferFinal[384];
        serializeJson(docEnvio, bufferFinal);

        // Envía el JSON procesado final
        enviarODefinirBuffer(String(bufferFinal));

        ultimoEstadoEnviado = diagnostico;
        ultimoEnvioMQTT = tiempoActual;
        
        acumuladoF1 = 0;
        acumuladoF2 = 0;

      } else {
        Serial.print("."); // Indicador visual de que el programa sigue corriendo pero no cumple tiempo de envío
      }

    } else {
      Serial.println("[ESP-NOW ERROR] El formato JSON recibido está corrupto o incompleto.");
    }

    digitalWrite(LED_PIN, LOW);
    hayDatoNuevo = false;
  }
}