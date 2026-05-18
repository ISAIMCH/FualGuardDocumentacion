# CONTRIBUTING.md

## 🤝 Contribuyendo a FuelGuard

¡Gracias por tu interés en contribuir! Este documento describe cómo participar en el proyecto.

---

## 📋 Requisitos Previos

- Conocimiento de Arduino/C++
- Familiaridad con Git
- Entorno Arduino IDE configurado
- Hardware: TTGO LoRa32 o similar

---

## 🔄 Proceso de Contribución

### 1. Fork y Clone

```bash
# Fork en GitHub
git clone https://github.com/tu-usuario/FuelGuard.git
cd FuelGuard
git checkout -b feature/tu-caracteristica
```

### 2. Cambios Locales

```bash
# Editar archivos
# Probar cambios en hardware
# Verificar que no rompe nada existente
```

### 3. Commit y Push

```bash
git add .
git commit -m "Add: descripción clara del cambio"
git push origin feature/tu-caracteristica
```

### 4. Pull Request

- Ir a GitHub
- Crear PR contra rama `main`
- Descripción detallada del cambio
- Referencias a issues relacionados

---

## ✅ Guía de Estilo

### Código Arduino/C++

```cpp
// ✓ Bueno
void setupLoRa() {
  int state = radio.begin(915.0);
  if (state == RADIOLIB_ERR_NONE) {
    radio.setOutputPower(10);
  }
}

// ✗ Evitar
void setupLoRa(){int state = radio.begin(915.0);if (state == RADIOLIB_ERR_NONE){radio.setOutputPower(10);}}
```

### Comentarios

```cpp
// ✓ Claro
// Inicializa el módulo LoRa en frecuencia 915 MHz
void setupLoRa() { ... }

// ✗ Innecesario
// Setup LoRa
void setupLoRa() { ... }
```

### Nombres de Variables

```cpp
// ✓ Descriptivo
float caudalimetro1_lps;
uint32_t ultimo_envio_timestamp;

// ✗ Ambiguo
float f1;
uint32_t t;
```

### Commits

```
✓ Bueno:
  - "Add: soporte para nuevo sensor de temperatura"
  - "Fix: crash en inicialización GPS"
  - "Refactor: optimizar código de LoRa"

✗ Evitar:
  - "cambios"
  - "fixed stuff"
  - "wip"
```

---

## 🧪 Testing

Antes de PR, verifica:

```bash
# 1. Compilación sin errores
[ ] Trasmisor.ino compila
[ ] Receptor.ino compila
[ ] Lilygo-P.ino compila

# 2. Funcionalidad
[ ] Sensores se leen correctamente
[ ] LoRa transmite/recibe
[ ] MQTT publica datos
[ ] Dashboard muestra actualizaciones

# 3. Cobertura
[ ] Sin memory leaks
[ ] Performance aceptable
[ ] Seguro en el hardware
```

---

## 🐛 Reportar Bugs

Crear issue con:

```markdown
## Descripción
Breve descripción del problema

## Pasos para reproducir
1. ...
2. ...
3. ...

## Comportamiento esperado
Qué debería pasar

## Comportamiento actual
Qué está pasando

## Hardware
- Placa: TTGO LoRa32 v2.1
- OS: Arduino IDE 1.8.x
- Versión FW: 1.0.0

## Logs
[Incluir salida serial o stack trace]
```

---

## 💡 Sugerir Mejoras

Crear issue con `[FEATURE]`:

```markdown
## [FEATURE] Título descriptivo

### Problema que resuelve
Descripción del problema

### Solución propuesta
Cómo lo resolvería

### Ejemplos adicionales
Casos de uso

### ¿Es breaking change?
[ ] Sí [ ] No
```

---

## 📦 Áreas de Contribución

### Firmware
- Optimización de consumo energético
- Nuevos protocolos de comunicación
- Soporte para más sensores
- Mejora de GPS/GNSS

### Software
- Dashboard web mejorado
- API RESTful
- Aplicación móvil
- Análisis avanzado

### Documentación
- Guías de instalación
- Tutoriales de sensores
- Traducción a otros idiomas
- Ejemplos de código

### Testing
- Suite de pruebas unitarias
- Pruebas de integración
- Documentación de cobertura

---

## 🏆 Código de Conducta

- Sé respetuoso y constructivo
- No discriminación de ningún tipo
- Cuestiona ideas, no personas
- Acepta crítica constructiva

---

## 📞 Contacto

- **Discord:** [Link]
- **Email:** [email]
- **Issues:** GitHub Issues
- **Discussions:** GitHub Discussions

---

¡Gracias por tu tiempo y esfuerzo! 🙌
