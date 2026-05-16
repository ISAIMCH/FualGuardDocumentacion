#define TINY_GSM_MODEM_SIM7070
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

float lat = 0, lon = 0;

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

  WiFi.mode(WIFI_STA);
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
    Serial.println("[ERROR] Módem no responde");
  }

  modem.sendAT("+CGPS=1");
  modem.waitResponse();

  modem.gprsConnect(apn, gprsUser, gprsPass);

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
  }
  esp_now_register_recv_cb(onDataRecv);

  mqtt.setServer(broker, 1883);
}

// ===================== FUNCIONES =====================

// 🔄 Reconectar red + internet
void asegurarConexion() {
  if (!modem.isNetworkConnected()) {
    Serial.println("[RED] Sin señal...");
    if (!modem.waitForNetwork(10000)) {
      Serial.println("[RED] No disponible");
      return;
    }
  }

  if (!modem.isGprsConnected()) {
    Serial.println("[GPRS] Reconectando...");
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      Serial.println("[GPRS] Fallo");
      return;
    } else {
      Serial.println("[GPRS] OK");
    }
  }
}

// 🔄 Reconectar MQTT
void asegurarMQTT() {
  if (!mqtt.connected()) {
    Serial.print("[MQTT] Reconectando...");
    if (mqtt.connect("GatewayIsaiITSOEH")) {
      Serial.println("OK");
    } else {
      Serial.println("Fallo");
    }
  }
}

// 📤 Enviar dato o guardar en buffer
void enviarODefinirBuffer(String data) {
  if (mqtt.publish(topicPublish, data.c_str())) {
    Serial.println("[MQTT OK] Enviado:");
    Serial.println(data);
  } else {
    Serial.println("[BUFFER] Guardando dato (sin conexion MQTT)...");
    if (bufferIndex < MAX_BUFFER) {
      bufferDatos[bufferIndex++] = data;
    } else {
      Serial.println("[BUFFER LLENO] Se pierde el dato mas antiguo");
    }
  }
}

// 🔁 Reenviar buffer
void enviarBufferPendiente() {
  if (bufferIndex == 0) return;

  Serial.println("[REINTENTO] Enviando buffer...");
  int enviadosConExito = 0;

  for (int i = 0; i < bufferIndex; i++) {
    if (mqtt.publish(topicPublish, bufferDatos[i].c_str())) {
      Serial.println("[REENVIADO OK]");
      enviadosConExito++;
    } else {
      Serial.println("[ERROR REENVIO] MQTT no disponible aun.");
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
  asegurarConexion();
  asegurarMQTT();
  mqtt.loop();

  enviarBufferPendiente();

  if (hayDatoNuevo) {
    digitalWrite(LED_PIN, HIGH);

    // Obtener GPS del módem de la LilyGO
    if (modem.getGPS(&lat, &lon)) {
      Serial.printf("[GPS SIM] %.6f, %.6f\n", lat, lon);
    }

    StaticJsonDocument<384> doc; 
    DeserializationError error = deserializeJson(doc, payloadESPNow);
    
    if (!error) {
      // 1. Extraer variables INSTANTÁNEAS para evaluar el peligro
      int f1 = doc["flujo1"];
      int f2 = doc["flujo2"];
      int reed = doc["reed"]; // 0 = ABIERTA, 1 = CERRADA

      // -------------------------------------------------------------
      // 🚀 NUEVO: ACUMULADORES SILENCIOSOS
      // -------------------------------------------------------------
      static unsigned long acumuladoF1 = 0;
      static unsigned long acumuladoF2 = 0;
      
      // Sumamos el consumo a nuestra "alcancía"
      acumuladoF1 += f1;
      acumuladoF2 += f2;

      // 2. Aplicar la Matriz de Estados Inteligente (con los datos instantáneos)
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

      // -------------------------------------------------------------
      // ⏱️ NUEVO: TEMPORIZADOR DINÁMICO (Ahorro de datos y GPS)
      // -------------------------------------------------------------
      static String ultimoEstadoEnviado = "";
      static unsigned long ultimoEnvioMQTT = 0;
      unsigned long tiempoActual = millis();
      
      // Definimos cada cuánto tiempo transmitiremos según el estado del camión
      unsigned long intervaloTransmision = 300000; // Por defecto: 5 minutos (300,000 ms)

      if (diagnostico == "OPERACION_NORMAL") {
        intervaloTransmision = 60000;  // Camión en movimiento: Transmite cada 1 minuto (para trazar el mapa GPS)
      } else if (esAlertaCritica) {
        intervaloTransmision = 3000;   // Alerta Crítica: Transmite en vivo cada 3 segundos
      }

      // 3. Evaluar si es momento de transmitir el paquete acumulado
      if (esAlertaCritica || (diagnostico != ultimoEstadoEnviado) || (tiempoActual - ultimoEnvioMQTT >= intervaloTransmision) || ultimoEnvioMQTT == 0) {
        
        // ¡Magia! Sobreescribimos el JSON con los totales acumulados antes de enviarlo
        doc["flujo1"] = acumuladoF1;
        doc["flujo2"] = acumuladoF2;

        // Inyectamos las coordenadas del Gateway celular y el estado
        doc["lat_sim"] = lat;
        doc["lon_sim"] = lon;
        doc["gateway"] = "Tanque-01"; 
        doc["status"] = diagnostico; 

        char bufferFinal[384];
        serializeJson(doc, bufferFinal);

        // Enviamos el paquete gigante a Node-RED e InfluxDB
        enviarODefinirBuffer(String(bufferFinal));

        // Reiniciamos los temporizadores y nuestra "alcancía"
        ultimoEstadoEnviado = diagnostico;
        ultimoEnvioMQTT = tiempoActual;
        
        acumuladoF1 = 0;
        acumuladoF2 = 0;

      } else {
        // En lugar de imprimir cada 3 segundos, lo mostramos ocasionalmente para no saturar el monitor
        Serial.print("."); 
      }

    } else {
      Serial.println("[ERROR JSON] Formato invalido recibido por ESP-NOW");
    }

    digitalWrite(LED_PIN, LOW);
    hayDatoNuevo = false;
  }
}