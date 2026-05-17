# FuelGuard — Sistema de Monitoreo de Combustible (LoRa / TTGO)

Proyecto FuelGuard: solución de hardware y software para monitorear consumo y seguridad de tanques de combustible usando dispositivos TTGO LoRa, gateways 4G y un panel web.

## Contenido del repositorio

- [Trasmisor](Trasmisor/) — Firmware del transmisor (LoRa) y documentación por subsistema.
- [Receptor](Receptor/) — Firmware del receptor/relay.
- [Lilygo-P](Lilygo-P/) — Código y configuración para gateway 4G.
- [PaginaWeb](PaginaWeb/) — Dashboard y recursos web.
- [Img](Img/) — Imágenes y diagramas usados en la documentación.
- [QUICKSTART.md](QUICKSTART.md) — Inicio rápido.
- [SETUP.md](SETUP.md) — Instrucciones de instalación detalladas.
- [TECHNICAL_SPECS.md](TECHNICAL_SPECS.md) — Especificaciones técnicas.
- [FAQ.md](FAQ.md) — Preguntas frecuentes.
- [CONTRIBUTING.md](CONTRIBUTING.md) — Cómo contribuir.
- [CHANGELOG.md](CHANGELOG.md) — Historial de versiones.
- [LICENSE.md](LICENSE.md) — Licencia del proyecto.

## Resumen rápido

- Hardware principal: TTGO LoRa32 v2.1 (SX1262), GPS NEO, caudalímetros, OLED.
- Comunicación: LoRa (915 MHz) entre transmisor y receptor; gateway 4G opcional para uplink.
- Formato de datos: JSON con campos de flujo, GPS, estado de tapa y métricas LoRa.

## Instalación rápida (5 minutos)

1. Clona el repositorio: `git clone <repo>`
2. Abrir `Trasmisor/Trasmisor.ino` en Arduino IDE.
3. Agregar URL de placas ESP32 en preferencias y seleccionar `ESP32 Dev Module`.
4. Cargar el sketch a la placa TTGO.
5. Revisar serial a `115200` baudios.

Para más detalles siga: [QUICKSTART.md](QUICKSTART.md) y [SETUP.md](SETUP.md).

## Arquitectura

1. Dispositivos transmisores (TTGO LoRa) → envían JSON vía LoRa.
2. Receptor/Gateway LoRa → recibe y reenvía (opcionalmente via 4G/MQTT).
3. Backend / Dashboard → visualiza métricas, almacenadas en InfluxDB/Grafana (opcional).

![Flujo Node-RED](Img/Flujo.png)

## Uso de imágenes

Las imágenes se encuentran en la carpeta `Img/` y están referenciadas desde este README y los documentos relacionados.

Ejemplos:

- [Diagrama Node-RED](Img/DashboardNodeRed.png)
- [Grafana example](Img/GraficosGrafana.png)

## Limpieza realizada

- Se movió la documentación específica del transmisor a [Trasmisor/README.md](Trasmisor/README.md).
- Se eliminaron archivos claramente redundantes o temporales para dejar solo la documentación principal.

## Siguientes pasos recomendados

1. Revisar [SETUP.md](SETUP.md) y [TECHNICAL_SPECS.md](TECHNICAL_SPECS.md) para detalles de montaje y calibración.
2. Probar el flujo end-to-end: transmitir datos → receptor → backend.
3. Si desea, puedo:
   - Ejecutar pruebas básicas (serial/logs) en los sketches.
   - Comprimir la documentación final lista para subir a GitHub.

---

Si quiere que borre o archive más archivos (por ejemplo páginas del dashboard), indíquelo y lo hago.
