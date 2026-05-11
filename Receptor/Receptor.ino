#include <heltec_unofficial.h>
#include <ArduinoJson.h>

String received;

// Control de señal
unsigned long lastPacketTime = 0;
const int timeout = 10000; // 10 segundos sin señal

void setup() {
  heltec_setup();
  Serial.begin(115200);

  // ===== LoRa (Usando heltec_unofficial) =====
  Serial.print("Iniciando Receptor LoRa... ");
  int radiostate = radio.begin(915.0); // Frecuencia 915 MHz

  if (radiostate == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa OK!");
  } else {
    Serial.print("Error LoRa, codigo: ");
    Serial.println(radiostate);
    while (true);
  }

  // 🔥 MISMA CONFIGURACIÓN QUE EL EMISOR (SF9)
  radio.setSpreadingFactor(9);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);

  Serial.println("Receptor listo");
  
  display.clear();
  display.drawString(0, 0, "CABINA RX LISTA");
  display.display();
}

// ===================== LOOP =====================
void loop() {
  heltec_loop();

  // radio.receive() intenta leer. Si no hay nada, regresa rápido para no bloquear el código.
  int state = radio.receive(received);

  if (state == RADIOLIB_ERR_NONE) {
    // ===== PAQUETE RECIBIDO CON ÉXITO =====
    lastPacketTime = millis();

    Serial.println("\n===== JSON RECIBIDO =====");
    Serial.println(received);

    // ===== JSON =====
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, received);

    if (error) {
      Serial.println("Error JSON");
      return; // Si el JSON está roto, sale de esta iteración y espera el siguiente
    }

    // Leemos los valores asegurándonos de que coincidan con los nombres que manda el TX
    float flujo1 = doc["flujo1"];
    float flujo2 = doc["flujo2"];
    float lat = doc["lat"];
    float lng = doc["lng"];
    int alerta = doc["alerta"];

    // ===== DISPLAY =====
    display.clear();

    display.drawString(0, 0, "CABINA RX");

    // Mostramos ambos flujos
    display.drawString(0, 12, "F1: " + String(flujo1, 1) + " F2: " + String(flujo2, 1));

    display.drawString(0, 24, alerta ? "TAPA: CERRADA" : "TAPA: ABIERTA");

    // Coordenadas
    display.drawString(0, 36, "Lat: " + String(lat, 4));
    display.drawString(0, 46, "Lng: " + String(lng, 4));

    display.display();

  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // Esto es muy útil: te avisa si la señal de radio llegó corrupta (interferencia)
    Serial.println("Paquete recibido pero corrupto (Error CRC)");
  }

  // ===== ALERTA DE SIN SEÑAL =====
  if (millis() - lastPacketTime > timeout) {
    display.clear();
    display.drawString(0, 0, "SIN SENAL LORA");
    display.drawString(0, 16, "Verificando TX...");
    display.display();
  }
}
