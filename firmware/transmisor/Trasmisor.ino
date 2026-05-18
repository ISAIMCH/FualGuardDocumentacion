#include <U8g2lib.h>

#include <Wire.h>

#include <RadioLib.h>

#include <SPI.h>

#include <TinyGPSPlus.h>

#include <ArduinoJson.h>

#include <esp_now.h>

#include <WiFi.h>    



// --- Definición de Pines OLED ---

#define OLED_SDA 17

#define OLED_SCL 18

#define OLED_RST 21

#define VEXT     36

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);



// --- Definición de Pines LoRa (SX1262) ---

#define LORA_SCK  9

#define LORA_MISO 11

#define LORA_MOSI 10

#define LORA_CS   8

#define LORA_DIO1 14

#define LORA_RST  12

#define LORA_BUSY 13

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);



// --- Pines Sensores Físicos ---

#define FLUJO1_PIN 4

#define FLUJO2_PIN 5

#define REED_PIN 6



// --- Pines GPS ---

#define GPS_RX_PIN 48

#define GPS_TX_PIN 47



// --- DIRECCIÓN MAC DEL RECEPTOR (Gateway SIM7070) ---

uint8_t direccionMacDestino[] = {0x10, 0x06, 0x1C, 0x40, 0xC4, 0x44};



// --- Variables para los Caudalímetros (Volátiles) ---

volatile unsigned int pulsosFlujo1 = 0;

volatile unsigned int pulsosFlujo2 = 0;

unsigned long tiempoAnterior = 0;



void IRAM_ATTR contarFlujo1() { pulsosFlujo1++; }

void IRAM_ATTR contarFlujo2() { pulsosFlujo2++; }



// --- Variables para el GPS ---

TinyGPSPlus gps;

HardwareSerial SerialGPS(1);



// --- Callback de envío ESP-NOW ---

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {

  Serial.print("Estado ESP-NOW: ");

  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "ENTREGADO" : "FALLÓ");

}



void setup() {

  Serial.begin(115200);



  // 1. Activar energía de periféricos

  pinMode(VEXT, OUTPUT);

  digitalWrite(VEXT, LOW);

  delay(100);



  // 2. Inicializar Pantalla

  pinMode(OLED_RST, OUTPUT);

  digitalWrite(OLED_RST, HIGH); delay(10);

  digitalWrite(OLED_RST, LOW); delay(10);

  digitalWrite(OLED_RST, HIGH); delay(50);

  display.begin();



  // 3. Inicializar Bus SPI y LoRa

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  int state = radio.begin(915.0);

  if (state == RADIOLIB_ERR_NONE) {

    radio.setOutputPower(10);

    radio.setSpreadingFactor(9);

  } else {

    while (true);

  }



  // 4. Configurar Configuración Inalámbrica de ESP-NOW

  WiFi.mode(WIFI_STA);

  WiFi.disconnect();



  if (esp_now_init() != ESP_OK) {

    Serial.println("Error inicializando ESP-NOW");

  } else {

    esp_now_register_send_cb(onDataSent);

   

    esp_now_peer_info_t peerInfo = {};

    memcpy(peerInfo.peer_addr, direccionMacDestino, 6);

    peerInfo.channel = 0;  

    peerInfo.encrypt = false;

   

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {

      Serial.println("Fallo al agregar Peer Receptora");

    }

  }



  // 5. Configurar Pines de Sensores

  pinMode(FLUJO1_PIN, INPUT_PULLUP);

  pinMode(FLUJO2_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(FLUJO1_PIN), contarFlujo1, RISING);

  attachInterrupt(digitalPinToInterrupt(FLUJO2_PIN), contarFlujo2, RISING);



  pinMode(REED_PIN, INPUT_PULLUP);

 

  // 6. Inicialización GPS

  SerialGPS.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

}



void loop() {

  while (SerialGPS.available() > 0) {

    gps.encode(SerialGPS.read());

  }



  if (millis() - tiempoAnterior > 3000) {

    tiempoAnterior = millis();



    // Lectura atómica de flujos

    noInterrupts();

    int lecturaFlujo1 = pulsosFlujo1;

    int lecturaFlujo2 = pulsosFlujo2;

    pulsosFlujo1 = 0;

    pulsosFlujo2 = 0;

    interrupts();



    // Filtro anti-ruido (Debounce) por muestras de votación

    int lecturasBajas = 0;

    for (int i = 0; i < 10; i++) {

      if (digitalRead(REED_PIN) == LOW) {

        lecturasBajas++;

      }

      delay(2);

    }

   

    // --- 🔥 CAMBIO SOLICITADO: INVERSIÓN DE LÓGICA REED SWITCH ---

    // Físicamente: Imán Cerca = pin lee LOW. Imán Lejos = pin lee HIGH.

    // Lógica nueva: Si la mayoría fue LOW (Imán cerca), se define como CERRADO (1).

    // De lo contrario, se define como ABIERTO (0).

    int estadoPuerta = (lecturasBajas > 5) ? 1 : 0;



    // Extracción de coordenadas GPS

    float lat = 0.0;

    float lng = 0.0;

    int sats = 0;

    int gps_ok = 0;



    if (gps.location.isValid()) {

      lat = gps.location.lat();

      lng = gps.location.lng();

      sats = gps.satellites.value();

      gps_ok = 1;

    }



    // Construcción del documento JSON

    StaticJsonDocument<256> doc;

    doc["id"] = "nodo1";

    doc["flujo1"] = lecturaFlujo1;

    doc["flujo2"] = lecturaFlujo2;

    doc["reed"] = estadoPuerta; // Mandará 0 si está abierto, 1 si está cerrado

    doc["lat"] = serialized(String(lat, 6));

    doc["lng"] = serialized(String(lng, 6));

    doc["sats"] = sats;

    doc["gps"] = gps_ok;



    String paqueteJSON;

    serializeJson(doc, paqueteJSON);



    // Transmisión multiprotocolo simultánea

    radio.transmit(paqueteJSON);

    esp_now_send(direccionMacDestino, (uint8_t *) paqueteJSON.c_str(), paqueteJSON.length());



    // Actualización visual en pantalla OLED local

    display.clearBuffer();

    display.setFont(u8g2_font_ncenB08_tr);

    display.drawStr(0, 12, "TX Hibrido Activo");

   

    String flujos = "F1: " + String(lecturaFlujo1) + " | F2: " + String(lecturaFlujo2);

    display.drawStr(0, 27, flujos.c_str());

   

    if (gps_ok) {

      String infoGps = "GPS OK (" + String(sats) + " sats)";

      display.drawStr(0, 42, infoGps.c_str());

    } else {

      display.drawStr(0, 42, "Buscando GPS...");

    }

   

    // Ajuste del texto en pantalla para reflejar la correspondencia de la nueva lógica

    display.drawStr(0, 57, (estadoPuerta == 1) ? "Puerta: CERRADA" : "Puerta: ABIERTA");

    display.sendBuffer();

   

    Serial.print("Transmitiendo JSON: ");

    Serial.println(paqueteJSON);

  }

}