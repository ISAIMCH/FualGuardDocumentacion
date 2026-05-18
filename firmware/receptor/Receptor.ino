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

// Factor de calibración (Pulsos a Litros)
const float FACTOR_CALIBRACION = 450.0;

// ==========================================
// NUEVO: Variables para la ventana de 5 mins
// ==========================================
unsigned long acumFlujo1 = 0;
unsigned long acumFlujo2 = 0;
unsigned long tiempoInicioVentana = 0;
const unsigned long TIEMPO_VENTANA = 300000; // 300,000 ms = 5 Minutos

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

  // Pantalla de bienvenida
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB12_tr); 
  display.drawStr(20, 35, "FuelGuard");
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(32, 55, "Iniciando...");
  display.sendBuffer();

  // 3. Inicializar LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(915.0);
  
  if (state == RADIOLIB_ERR_NONE) {
    radio.setSpreadingFactor(9); 
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB10_tr);
    display.drawStr(20, 25, "FuelGuard");
    display.drawStr(0, 50, "LoRa SF9: EN LINEA");
    display.sendBuffer();
    delay(1500);
    
    // 🔥 Arrancamos el cronómetro de 5 minutos
    tiempoInicioVentana = millis(); 
  } else {
    display.clearBuffer();
    display.drawStr(0, 30, "Error Hardware LoRa");
    display.sendBuffer();
    while (true); 
  }
}

void loop() {
  // ==========================================
  // REINICIO AUTOMÁTICO CADA 5 MINUTOS
  // ==========================================
  if (millis() - tiempoInicioVentana >= TIEMPO_VENTANA) {
    acumFlujo1 = 0; // Borramos la memoria de flujo de ida
    acumFlujo2 = 0; // Borramos la memoria de flujo de retorno
    tiempoInicioVentana = millis(); // Reiniciamos el cronómetro
    Serial.println("\n[INFO] Ventana de 5 minutos reiniciada a cero.");
  }

  String paqueteRecibido;
  int state = radio.receive(paqueteRecibido);

  if (state == RADIOLIB_ERR_NONE) {
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, paqueteRecibido);

    if (!error) {
      // 1. Extraer los datos crudos del paquete entrante
      int flujo1 = doc["flujo1"];
      int flujo2 = doc["flujo2"];
      int reedStatus = doc["reed"]; 
      String lat = doc["lat"];
      String lng = doc["lng"];
      int gps_ok = doc["gps"];

      // 2. Acumular los pulsos en nuestras variables de memoria de 5 minutos
      acumFlujo1 += flujo1;
      acumFlujo2 += flujo2;

      // 3. CONVERSIÓN A LITROS DEL ACUMULADO
      float litrosIda = acumFlujo1 / FACTOR_CALIBRACION;
      float litrosRetorno = acumFlujo2 / FACTOR_CALIBRACION;
      float litrosNeto = max(0.0f, litrosIda - litrosRetorno); 

      // --- RENDERIZADO DE LA INTERFAZ DE USUARIO (UI) ---
      display.clearBuffer();
      
      // Fila 1: Encabezado Corporativo
      display.setFont(u8g2_font_ncenB10_tr); 
      display.drawStr(26, 12, "FuelGuard"); 
      display.drawLine(0, 16, 128, 16);
      
      // Fila 2: Consumo Principal (Añadido "(5m)" para que el usuario entienda el lapso)
      display.setFont(u8g2_font_ncenB08_tr); 
      String txtConsumo = "Gasto(5m): " + String(litrosNeto, 2) + " L";
      display.drawStr(0, 31, txtConsumo.c_str());
      
      // Fila 3: Desglose de Flujos 
      String txtDesglose = "In:" + String(litrosIda, 1) + "L | Out:" + String(litrosRetorno, 1) + "L";
      display.drawStr(0, 43, txtDesglose.c_str());
      
      // Fila 4: Alerta de la Tapa
      if (reedStatus == 1) {
        display.drawStr(0, 54, "Tapa: SEGURA (Ok)");
      } else {
        display.drawStr(0, 54, "Tapa: !ABIERTA!"); 
      }
      
      // Fila 5: MOSTRAR LAS COORDENADAS
      // Usamos una fuente compacta para que latitud y longitud quepan en 128 pixeles
      display.setFont(u8g2_font_6x10_tf); 
      if (gps_ok == 1) {
        // Reducimos las coordenadas a 8 caracteres (ej. 19.4326, -99.1332)
        String gpsTxt = lat.substring(0, 8) + ", " + lng.substring(0, 8);
        display.drawStr(0, 64, gpsTxt.c_str());
      } else {
        display.drawStr(0, 64, "Buscando coordenadas...");
      }
      
      // Actualizar Pantalla
      display.sendBuffer();

      // --- Monitor Serie (Solo respaldo técnico) ---
      Serial.printf("--- DATOS ACUMULADOS (Lapso de 5 mins) ---\n");
      Serial.printf("Gasto Neto: %.2f L\n", litrosNeto);
      if (gps_ok) Serial.println("Posicion: " + lat + ", " + lng);
    }
  }
}