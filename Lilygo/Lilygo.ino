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
// NOTA: Si usas ESP32 Core v3.x, cambia la linea de abajo por:
// void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
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
      // Opcional: Rotar el buffer (descartar el mas viejo en lugar del nuevo)
      // Para mantenerlo simple, solo omitimos guardar el nuevo.
    }
  }
}

// 🔁 Reenviar buffer (Corregido)
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
      break; // Rompemos el ciclo en el primer error para no seguir fallando
    }
  }

  // Si logramos enviar algunos datos (o todos), recorremos el arreglo 
  // para no perder los que no se pudieron enviar y no duplicar los enviados.
  if (enviadosConExito > 0) {
    int restantes = bufferIndex - enviadosConExito;
    for (int i = 0; i < restantes; i++) {
      bufferDatos[i] = bufferDatos[i + enviadosConExito];
    }
    bufferIndex = restantes; // Actualizamos el tamaño del buffer
  }
}

// ===================== LOOP =====================
void loop() {

  asegurarConexion();
  asegurarMQTT();
  mqtt.loop();

  // Intentamos reenviar lo que este atascado en el buffer
  enviarBufferPendiente();

  if (hayDatoNuevo) {
    digitalWrite(LED_PIN, HIGH);

    Serial.println("\n[ESP-NOW Recibido]");
    Serial.println(payloadESPNow);

    // GPS del SIM7070
    if (modem.getGPS(&lat, &lon)) {
      Serial.printf("[GPS SIM] %.6f, %.6f\n", lat, lon);
    }

    // JSON
    // NOTA: Usa DynamicJsonDocument si estas en ArduinoJson v6, o JsonDocument en v7.
    StaticJsonDocument<256> doc; 
    
    // Deserializamos. Si no hay error (!error), inyectamos la data.
    DeserializationError error = deserializeJson(doc, payloadESPNow);
    
    if (!error) {
      doc["lat_sim"] = lat;
      doc["lon_sim"] = lon;
      
      // Agregamos un identificador de Gateway para no borrar el "id" del nodo Heltec
      doc["gateway"] = "Tanque-01"; 

      char bufferFinal[256];
      serializeJson(doc, bufferFinal);

      enviarODefinirBuffer(String(bufferFinal));
    } else {
      Serial.println("[ERROR JSON] Formato invalido recibido por ESP-NOW");
    }

    digitalWrite(LED_PIN, LOW);
    hayDatoNuevo = false;
  }
}