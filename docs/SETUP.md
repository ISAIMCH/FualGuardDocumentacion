# FuelGuard - Firmware & Hardware Setup

## 📦 Instalación de Dependencias Arduino

### Paso 1: Configurar Arduino IDE

1. Descargar [Arduino IDE 1.8.13+](https://www.arduino.cc/en/software)
2. Instalar drivers CH340:
   - **Windows:** [CH340 Driver](http://www.wch.cn/download/ch341ser_exe.zip)
   - **Mac:** [CH340 Mac Driver](http://www.wch.cn/download/ch341ser_mac_zip.zip)
   - **Linux:** `sudo apt-get install ch340`

### Paso 2: Agregar Board Manager

En Arduino IDE:
- Preferencias → URLs adicionales de gestor de tarjetas
- Agregar: `https://dl.espressif.com/dl/package_esp32_index.json`
- Aceptar

### Paso 3: Instalar Board ESP32

- Herramientas → Placa → Gestor de tarjetas
- Buscar: "esp32"
- Instalar "ESP32 by Espressif Systems" v2.0.x

### Paso 4: Seleccionar Configuración

**Para TTGO LoRa32 v2.1:**

```
Placa: ESP32 Dev Module
Upload Speed: 921600
CPU Frequency: 240MHz
Flash Frequency: 80MHz
Flash Mode: QIO
Flash Size: 4MB
Partition Scheme: Default
Core Debug Level: None
```

### Paso 5: Instalar Librerías

En Arduino IDE → Sketch → Include Library → Manage Libraries

Copiar y pegar cada nombre en la búsqueda:

```
1. RadioLib                    [6.1.0]
2. TinyGSM                     [0.11.4]
3. PubSubClient                [2.8.0]
4. ArduinoJson                 [6.19.0]
5. U8g2lib                     [2.32.6]
6. TinyGPSPlus                 [1.0.3]
7. Time                        [1.6.1]
```

**Instalación alternativa (copiar a carpeta libraries):**

```bash
# Descargar y extraer ZIP en:
# Windows: Documents\Arduino\libraries\
# Mac: ~/Documents/Arduino/libraries/
# Linux: ~/Arduino/libraries/
```

---

## 🔌 Conexión de Hardware

### Trasmisor & Receptor (TTGO LoRa32)

#### Sensores Digitales

```
Caudalímetro 1:
  - Puerto: GPIO 4 (INPUT)
  - Señal: Pulsos por segundo
  - Alimentación: 3.3V
  
Caudalímetro 2:
  - Puerto: GPIO 5 (INPUT)
  - Señal: Pulsos por segundo
  - Alimentación: 3.3V

Sensor Reed (Tapa):
  - Puerto: GPIO 6 (INPUT_PULLUP)
  - Señal: ALTO=Cerrado, BAJO=Abierto
  - Alimentación: 3.3V
```

#### GPS (UART)

```
GPS NEO-6M/NEO-M8N:
  - RX (In) → GPIO 47 (UART1 RX)
  - TX (Out) → GPIO 48 (UART1 TX)
  - 3V3 → Vcc
  - GND → GND
  - Antena: Orientada verticalmente
```

#### Pantalla OLED

```
SSD1306 128x64:
  - SDA → GPIO 17 (I2C)
  - SCL → GPIO 18 (I2C)
  - RST → GPIO 21 (Reset)
  - 3V3 → Vcc
  - GND → GND
```

### Gateway (TTGO T-A7 SIM7070)

#### Módulo 4G

```
SIM7070:
  - TX → UART (GPIO TXD)
  - RX → UART (GPIO RXD)
  - GND → GND
  - Vcc → 5V (adaptador)
  - Antena: Con ganancia 2dBi mínimo
```

#### Tarjeta SIM

```
- Insertar SIM activa en slot
- Verificar cobertura celular
- APN: web.iusacellgsm.mx
```

---

## 📱 Carga de Firmware

### Método 1: Arduino IDE (Recomendado)

```bash
# 1. Conectar dispositivo por USB-C
# 2. Seleccionar puerto COM en Herramientas
# 3. Cargar archivo .ino
# 4. Botón Verificar (✓)
# 5. Botón Cargar (→)

# Esperar "Se cargó el código exitosamente"
```

### Método 2: Herramientas de Línea de Comandos

```bash
# Instalar esptool
pip install esptool

# Compilar
arduino-cli compile --fqbn esp32:esp32:esp32 Trasmisor.ino

# Cargar
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash \
  0x1000 build/Trasmisor.ino.esp32.esp32.esp32.elf
```

### Método 3: Compilación Manual

```bash
# Clonar ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git

# Compilar
cd esp-idf && ./install.sh

# Cargar firmware compilado
```

---

## ✅ Verificación Post-Instalación

### Test 1: Monitor Serie

```bash
# Abrir Monitor → Velocidad 115200 baud

# Esperado en Trasmisor:
[SETUP] Inicializando...
[GPS] Buscando satélites...
[LORA] Módulo listo
[SENSOR] Flujo1: 0.00 L/s
```

### Test 2: Conectividad GPS

```bash
# Monitor serie debe mostrar:
[GPS] Satélites: 12
[GPS] Lat: 20.028306
[GPS] Lon: -99.225007
[GPS] Alt: 2250.5m
```

### Test 3: Comunicación LoRa

```bash
# Trasmisor transmite:
[LORA] TX 64 bytes

# Receptor recibe:
[LORA] RX: {"flujo1": 0.45, ...}
```

### Test 4: Conectividad 4G

```bash
# Gateway debe conectar:
[4G] Buscando red...
[4G] Conectado
[GPS] ubicación actual
[MQTT] Publicado
```

---

## 🧨 Reseteo y Recovery

### Hard Reset (Hardware)

```
TTGO LoRa32:
1. Presionar BOOT por 2 segundos
2. Presionar RST
3. Soltar BOOT
4. Dispositivo en modo DFU
```

### Borrar Flash

```bash
# Opción 1: Arduino IDE
Herramientas → Erase All Flash Before Sketch Upload

# Opción 2: esptool
esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash

# Opción 3: Manual
Mantener BOOT presionado durante upload
```

### Recuperación de Bootloader

```bash
# Si está corrompido:
esptool.py --chip esp32 --port /dev/ttyUSB0 \
  write_flash 0x1000 bootloader.bin
```

---

## 🔧 Troubleshooting

### "Puerto no disponible"

```bash
# Windows: Revisar Device Manager
devcon list ports

# Linux: Listar puertos
ls -la /dev/tty*

# Instalar drivers CH340
# Reiniciar IDE y computadora
```

### "Falla en verificación"

```
1. Bajar velocidad Upload: 115200
2. Usar cable USB corto
3. Desconectar periféricos USB
4. Probar otro puerto USB
5. Reinstalar drivers
```

### "Error en escaneo de puerto"

```bash
# Probar permisos en Linux
sudo usermod -a -G dialout $USER

# O usar:
sudo chmod 666 /dev/ttyUSB0
```

### "Módulo LoRa no responde"

```
1. Revisar pines SPI: SCK, MOSI, MISO
2. Verificar voltaje 3.3V en módulo
3. Antena conectada?
4. Probar con jumpers menos largos
5. Revisar en serial: [LORA] Error inicializando
```

---

## 📚 Recursos

- [Guía oficial TTGO](https://github.com/LilyGO/TTGO-LORA32)
- [Arduino Reference](https://www.arduino.cc/reference/en/)
- [Espressif ESP32 Docs](https://docs.espressif.com/)
- [RadioLib Documentation](https://jgromes.github.io/RadioLib/)

---

## 📞 Soporte

Si tienes problemas:

1. Revisar [FAQ.md](FAQ.md)
2. Buscar en [GitHub Issues](https://github.com/tu-usuario/FuelGuard/issues)
3. Crear nuevo issue con:
   - Placa utilizada
   - Error exacto
   - Pasos para reproducir
   - Salida del serial monitor

---

**Última actualización:** 16 de Mayo de 2026
