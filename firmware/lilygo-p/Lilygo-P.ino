#define TINY_GSM_MODEM_SIM7070
#define TINY_GSM_RX_BUFFER 1024 // IMPORTANTE: Previene cuelgues por respuestas largas del módem

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// ===================== PINES =====================
#define MODEM_TX      27
#define MODEM_RX      26
#define MODEM_PWRKEY  4
#define LED_PIN       12

// ===================== RED =====================
const char apn[]      = "web.iusacellgsm.mx";
const char gprsUser[] = "iusacellgsm";
const char gprsPass[] = "iusacellgsm";

const char* broker = "broker.hivemq.com";
const char* topicPublish = "itics/mgti/isai/sensor";

// ===================== OBJETOS =====================
HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

// ===================== VARIABLES =====================
String payloadESPNow = "";
volatile bool hayDatoNuevo = false;

// ===== BUFFER (almacena datos si no hay internet) =====
#define MAX_BUFFER 10
String bufferDatos[MAX_BUFFER];
int bufferIndex = 0;

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

  pinMode(LED_PIN, OUTPUT);
  pinMode(MODEM_PWRKEY, OUTPUT);

  // Asegurar que la WiFi no intente hacer reconexiones raras que afecten ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("MAC LilyGO: ");
  Serial.println(WiFi.macAddress());

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  // Encender módem
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(1500);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(5000);

  Serial.println("Iniciando módem...");

  if (!modem.testAT()) {
    Serial.println("[ERROR] Módem no responde. Revisa la batería y pines.");
  }

  // ELIMINADO: Todo el código de activación del GPS interno del SIM7070

  // ESP-NOW Init
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
  }
  esp_now_register_recv_cb(onDataRecv);

  mqtt.setServer(broker, 1883);
  mqtt.setKeepAlive(60); // Mantener la conexión MQTT viva por más tiempo
}

// ===================== FUNCIONES =====================

// 🔄 Reconectar red + internet (No bloqueante en caso de caída)
void asegurarConexion() {
  if (!modem.isNetworkConnected()) {
    Serial.println("[RED] Esperando señal celular...");
    if (!modem.waitForNetwork(60000)) {
      Serial.println("[RED] No disponible por ahora");
      return;
    }
  }

  if (modem.isNetworkConnected() && !modem.isGprsConnected()) {
    Serial.println("[GPRS] Conectando a datos...");
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      Serial.println("[GPRS] Fallo de conexión de datos");
      return;
    } else {
      Serial.println("[GPRS] CONECTADO OK");
    }
  }
}

// 🔄 Reconectar MQTT
void asegurarMQTT() {
  if (!mqtt.connected()) {
    Serial.print("[MQTT] Reconectando...");
    String clientId = "GatewayFuelGuard-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("OK");
    } else {
      Serial.print("Fallo, rc=");
      Serial.println(mqtt.state());
    }
  }
}

// 📤 Enviar dato o guardar en buffer
void enviarODefinirBuffer(String data) {
  if (mqtt.connected() && mqtt.publish(topicPublish, data.c_str())) {
    Serial.println("[MQTT OK] Paquete enviado.");
  } else {
    Serial.println("[BUFFER] Sin conexión MQTT. Guardando dato localmente...");
    if (bufferIndex < MAX_BUFFER) {
      bufferDatos[bufferIndex++] = data;
    } else {
      Serial.println("[BUFFER LLENO] Se pierde el dato más antiguo.");
      for (int i = 1; i < MAX_BUFFER; i++) {
        bufferDatos[i - 1] = bufferDatos[i];
      }
      bufferDatos[MAX_BUFFER - 1] = data;
    }
  }
}

// 🔁 Reenviar buffer
void enviarBufferPendiente() {
  if (bufferIndex == 0) return;

  Serial.println("[REINTENTO] Enviando buffer pendiente...");
  int enviadosConExito = 0;

  for (int i = 0; i < bufferIndex; i++) {
    if (mqtt.publish(topicPublish, bufferDatos[i].c_str())) {
      Serial.println("[REENVIADO OK] Paquete recuperado.");
      enviadosConExito++;
    } else {
      Serial.println("[ERROR REENVIO] Se perdió la conexión MQTT durante el reenvío.");
      break; 
    }
  }

  if (enviadosConExito > 0) {
    int restantes = bufferIndex - enviadosConExito;
    for (int i = 0; i < restantes; i++) {
      bufferDatos[i] = bufferDatos[i + enviadosConExito];
    }
    bufferIndex = restantes;
  }
}

// ===================== LOOP =====================
void loop() {
  // 1. Verificar el enlace GPRS
  asegurarConexion();

  // 2. SOLO si hay internet, operamos con MQTT
  if (modem.isGprsConnected()) {
    asegurarMQTT();
    if (mqtt.connected()) {
      mqtt.loop();
      enviarBufferPendiente();
    }
  }

  // 3. Procesamiento de la red sensora
  if (hayDatoNuevo) {
    digitalWrite(LED_PIN, HIGH);

    StaticJsonDocument<384> docRecibido;
    DeserializationError error = deserializeJson(docRecibido, payloadESPNow);
    
    if (!error) {
      // Extraemos los datos enviados por la Heltec Transmisora
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
        
        // Creamos el JSON que se irá a HiveMQ (Node-RED)
        StaticJsonDocument<384> docEnvio;
        docEnvio["flujo1"] = acumuladoF1;
        docEnvio["flujo2"] = acumuladoF2;
        docEnvio["lat"] = latitud_heltec;    // Usamos el GPS del Heltec
        docEnvio["lng"] = longitud_heltec;   // Usamos el GPS del Heltec
        docEnvio["gateway"] = "Tanque-01";
        docEnvio["status"] = diagnostico;

        char bufferFinal[384];
        serializeJson(docEnvio, bufferFinal);

        enviarODefinirBuffer(String(bufferFinal));

        ultimoEstadoEnviado = diagnostico;
        ultimoEnvioMQTT = tiempoActual;
        
        acumuladoF1 = 0;
        acumuladoF2 = 0;

      } else {
        Serial.print(".");
      }

    } else {
      Serial.println("[ERROR JSON] Formato inválido recibido por ESP-NOW");
    }

    digitalWrite(LED_PIN, LOW);
    hayDatoNuevo = false;
  }
}