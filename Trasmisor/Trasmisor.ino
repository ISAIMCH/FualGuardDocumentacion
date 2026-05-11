#include <heltec_unofficial.h>
#include <esp_now.h>
#include <WiFi.h>
#include <TinyGPS++.h>

// ===================== GPS =====================
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

#define GPS_RX 33
#define GPS_TX 4

// ===================== ESP-NOW =====================
uint8_t broadcastAddress[] = {0x10, 0x06, 0x1C, 0x40, 0xC4, 0x44};

// ===================== SENSORES =====================
#define PIN_SENSOR 5    // Reed switch
#define PIN_FLOW_1 38   // Sensor de flujo 1 (Con resistencia pull-up externa)
#define PIN_FLOW_2 39   // Sensor de flujo 2 (Con resistencia pull-up externa)

volatile long pulseCount1 = 0;
volatile long pulseCount2 = 0;
float flowRate1 = 0.0;
float flowRate2 = 0.0;
const float calibrationFactor = 5.5;

unsigned long lastSendTime = 0;
const int interval = 5000;

// ===================== INTERRUPCIONES =====================
void IRAM_ATTR pulseCounter1() {
  pulseCount1++;
}

void IRAM_ATTR pulseCounter2() {
  pulseCount2++;
}

// ===================== CALLBACK =====================
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "ESP-NOW OK" : "ESP-NOW FAIL");
}

// ===================== SETUP =====================
void setup() {
  heltec_setup();
  Serial.begin(115200);

  gpsSerial.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);

  // ===== ESP-NOW =====
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error agregando peer");
    return;
  }

  // ===== LoRa (Usando heltec_unofficial para V3) =====
  Serial.print("Iniciando LoRa... ");
  int radiostate = radio.begin(915.0); // Frecuencia 915 MHz
  
  if (radiostate == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa OK!");
  } else {
    Serial.print("Error LoRa, codigo: ");
    Serial.println(radiostate);
    while (true);
  }

  // 🔥 CONFIGURACIÓN IDEAL PARA TU CASO (CAMIÓN)
  radio.setSpreadingFactor(9);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);
  radio.setOutputPower(17);

  // ===== Pines =====
  pinMode(PIN_SENSOR, INPUT_PULLUP);
  
  // Como ya tienen resistencia física, usamos INPUT normal
  pinMode(PIN_FLOW_1, INPUT);
  pinMode(PIN_FLOW_2, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_1), pulseCounter1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_2), pulseCounter2, FALLING);

  display.clear();
  display.drawString(0, 0, "TX GPS + 2 FLOW");
  display.display();
}

// ===================== LOOP =====================
void loop() {
  heltec_loop();

  // Leer GPS
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (millis() - lastSendTime > interval) {

    // ===== SENSORES =====
    flowRate1 = (pulseCount1 / calibrationFactor);
    flowRate2 = (pulseCount2 / calibrationFactor);
    
    int estadoSensor = digitalRead(PIN_SENSOR);
    int alerta = (estadoSensor == LOW) ? 1 : 0;

    // ===== GPS =====
    float lat = 0, lng = 0;
    int sats = 0, gps_ok = 0;

    if (gps.location.isValid()) {
      lat = gps.location.lat();
      lng = gps.location.lng();
      sats = gps.satellites.value();
      gps_ok = 1;
    }

    // ===== JSON =====
    String json = "{";
    json += "\"id\":\"nodo1\",";
    json += "\"alerta\":" + String(alerta) + ",";
    json += "\"flujo1\":" + String(flowRate1, 2) + ",";
    json += "\"flujo2\":" + String(flowRate2, 2) + ",";
    json += "\"lat\":" + String(lat, 6) + ",";
    json += "\"lng\":" + String(lng, 6) + ",";
    json += "\"sats\":" + String(sats) + ",";
    json += "\"gps\":" + String(gps_ok);
    json += "}";

    // ===== ESP-NOW =====
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) json.c_str(), json.length());

    if (result == ESP_OK) {
      Serial.println("ESP-NOW enviado");
    } else {
      Serial.println("Error ESP-NOW");
    }

    // ===== LoRa =====
    int txState = radio.transmit(json);
    
    if (txState == RADIOLIB_ERR_NONE) {
      Serial.println("LoRa enviado con exito");
    } else {
      Serial.print("Error enviando LoRa: ");
      Serial.println(txState);
    }

    // ===== OLED =====
    display.clear();
    display.drawString(0, 0, "TX ACTIVO");
    display.drawString(0, 12, "F1: " + String(flowRate1,1) + " F2: " + String(flowRate2,1));
    display.drawString(0, 24, alerta ? "PUERTA: CERRADA" : "PUERTA: ABIERTA");

    if (gps_ok) {
      display.drawString(0, 36, "GPS OK (" + String(sats) + " sats)");
    } else {
      display.drawString(0, 36, "NO GPS");
    }

    display.display();

    // ===== DEBUG =====
    Serial.println("\nJSON:");
    Serial.println(json);

    // Reiniciar contadores
    pulseCount1 = 0;
    pulseCount2 = 0;
    lastSendTime = millis();
  }
}