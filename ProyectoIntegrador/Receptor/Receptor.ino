#include <U8g2lib.h>
#include <Wire.h>
#include <RadioLib.h>
#include <SPI.h>
#include <ArduinoJson.h> 

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

void setup() {
  Serial.begin(115200);

  // 1. Activar energía de periféricos (Pantalla y Radio)
  pinMode(VEXT, OUTPUT);
  digitalWrite(VEXT, LOW); 
  delay(100);

  // 2. Inicializar Pantalla
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, HIGH); delay(10);
  digitalWrite(OLED_RST, LOW); delay(10);
  digitalWrite(OLED_RST, HIGH); delay(50);
  display.begin();

  // Pantalla de bienvenida / Boot de FuelGuard
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB12_tr); // Fuente grande y elegante
  display.drawStr(20, 35, "FuelGuard");
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(32, 55, "Iniciando...");
  display.sendBuffer();

  // 3. Inicializar Bus SPI y módulo LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(915.0);
  
  if (state == RADIOLIB_ERR_NONE) {
    radio.setSpreadingFactor(9); // Forzar SF9 para acoplarse al transmisor
    
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB10_tr);
    display.drawStr(20, 25, "FuelGuard");
    display.drawStr(0, 50, "LoRa SF9: EN LINEA");
    display.sendBuffer();
    delay(1500);
  } else {
    display.clearBuffer();
    display.drawStr(0, 30, "Error Hardware LoRa");
    display.sendBuffer();
    while (true); 
  }
}

void loop() {
  String paqueteRecibido;
  
  // Escuchar constantemente el canal de radio
  int state = radio.receive(paqueteRecibido);

  if (state == RADIOLIB_ERR_NONE) {
    
    // Reservar espacio en memoria para decodificar el JSON entrante
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, paqueteRecibido);

    if (!error) {
      // Extraer datos usando las llaves del JSON del transmisor
      int flujo1 = doc["flujo1"];
      int flujo2 = doc["flujo2"];
      int reedStatus = doc["reed"]; // 1 = Cerrado, 0 = Abierto
      String lat = doc["lat"];
      String lng = doc["lng"];
      int sats = doc["sats"];
      int gps_ok = doc["gps"];

      // --- RENDERIZADO DE LA INTERFAZ DE USUARIO (UI) ---
      display.clearBuffer();
      
      // 1. Encabezado Corporativo "FuelGuard"
      display.setFont(u8g2_font_ncenB10_tr); // Fuente semi-negrita mediana
      display.drawStr(26, 12, "FuelGuard"); // Centrado aproximado en pixeles
      
      // Línea divisoria elegante justo debajo del título (X_inicio, Y_inicio, X_fin, Y_fin)
      display.drawLine(0, 16, 128, 16);
      
      // Cambiar a fuente estándar compacta para los datos técnicos
      display.setFont(u8g2_font_ncenB08_tr);

      // 2. Línea de Caudalímetros (Consumo Neto e Individual)
      // Calculamos la diferencia en pulsos directamente en el receptor
      int neto = flujo1 - flujo2; 
      String txtFlujos = "F1:" + String(flujo1) + " F2:" + String(flujo2) + " Net:" + String(neto);
      display.drawStr(0, 29, txtFlujos.c_str());
      
      // 3. Línea del Sensor Magnético (Respetando tu nueva lógica inversa: 1=Cerrado, 0=Abierto)
      if (reedStatus == 1) {
        display.drawStr(0, 42, "Tapa: CERRADA (OK)");
      } else {
        // Alerta visual: Añadimos signos de exclamación si abren el tanque
        display.drawStr(0, 42, "Tapa: ¡ABIERTA! 🚨"); 
      }
      
      // 4. Línea de Telemetría GPS y Satélites
      if (gps_ok == 1) {
        String gpsTxt = "GPS: OK (" + String(sats) + " satelites)";
        display.drawStr(0, 55, gpsTxt.c_str());
        
        // Opcional: Mostrar coordenadas resumidas en la última fila si cabe en pantalla
        String coordsShort = "L: " + lat.substring(0, 8) + " Lg: " + lng.substring(0, 8);
        display.setFont(u8g2_font_6x10_tf); // Fuente ultra compacta para que quepan las coordenadas enteras
        display.drawStr(0, 64, coordsShort.c_str());
      } else {
        display.drawStr(0, 55, "GPS: Sincronizando...");
      }
      
      // Enviar todo el búfer gráfico acumulado a los pixeles físicos de la OLED
      display.sendBuffer();

      // --- Copia de seguridad en el Monitor Serie de la PC ---
      Serial.println("--- DATOS RECIBIDOS EN FUELGUARD ---");
      Serial.printf("Flujo 1: %d | Flujo 2: %d | Neto: %d\n", flujo1, flujo2, neto);
      Serial.println(reedStatus == 1 ? "Tapa: Cerrada" : "Alerta: ¡Tapa Abierta!");
      Serial.println("Coordenadas: " + lat + ", " + lng + " (Sats: " + String(sats) + ")");
      Serial.println("Calidad de Enlace LoRa (RSSI): " + String(radio.getRSSI()) + " dBm\n");
    }
  }
}