#include <heltec_unofficial.h>
#include <LoRa.h>
#include <ArduinoJson.h>

String received;

void setup() {
  heltec_setup();
  Serial.begin(115200);

  if (!LoRa.begin(915E6)) {
    Serial.println("Error LoRa");
    while (true);
  }

  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  Serial.println("RX listo");
}

void loop() {
  heltec_loop();

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, received)) {
      Serial.println("Error JSON");
      return;
    }

    float flujo = doc["flujo"];
    float lat = doc["lat"];
    float lng = doc["lng"];
    int alerta = doc["alerta"];

    // ===== DISPLAY =====
    display.clear();

    display.drawString(0, 0, "CABINA RX");

    display.drawString(0, 12, "Flujo: " + String(flujo,1));

    display.drawString(0, 22,
      String("Tapa: ") + (alerta ? "CERRADA" : "ABIERTA")
    );

    display.drawString(0, 34, String(lat,4));
    display.drawString(0, 44, String(lng,4));

    display.display();

    Serial.println(received);
  }
}