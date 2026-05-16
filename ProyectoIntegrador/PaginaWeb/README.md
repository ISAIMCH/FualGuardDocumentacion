# PaginaWeb - Dashboard FuelGuard

## 📱 Descripción

Portal web responsivo que proporciona visualización en tiempo real de datos del sistema FuelGuard con autenticación y soporte para múltiples dispositivos.

## 🎯 Función Principal

Mostrar dashboard interactivo con datos MQTT en vivo, gráficas y estado del sistema.

## 📦 Contenido

```
PaginaWeb/
├── app.html              ← Login y autenticación
├── Dashboard.html        ← Redirect inteligente
├── Dashboar.html         ← Dashboard principal
└── README.md             ← Este archivo
```

## 🎨 Características de Diseño

### Paleta de Colores

```css
--bg: #07111f          /* Fondo oscuro */
--surface: rgba(255, 255, 255, 0.06)   /* Cards */
--text: #f5f7fb         /* Texto principal */
--muted: #a7b3c7        /* Texto secundario */
--line: rgba(255, 255, 255, 0.12)      /* Bordes */
--brand: #ff7a18        /* Naranja primario */
--brand-2: #ffb21e      /* Naranja secundario */
--accent: #31c48d       /* Verde acento */
```

### Tipografía

```
Font: Google Fonts "Inter"
Pesos: 400, 500, 600, 700, 800, 900
Fallback: -apple-system, BlinkMacSystemFont, "Segoe UI"
```

## 🚀 Inicio Rápido

### Opción 1: Servidor Local

```bash
# Python 3
python -m http.server 8000

# Node.js
npx http-server

# Luego abrir:
http://localhost:8000/app.html
```

### Opción 2: Servidor Remoto

```bash
# Copiar archivos a servidor web
# (Apache, Nginx, etc.)

# Acceder a:
http://tu-servidor.com/app.html
```

### Opción 3: Electron Desktop

```bash
# Crear aplicación desktop
# (implementar en versión v1.1)
```

## 📋 Estructura de Archivos

### app.html - Pantalla de Login

```html
├─ Head
│  ├─ Meta tags (viewport, charset)
│  ├─ Google Fonts
│  ├─ Font Awesome Icons
│  └─ CSS personalizado
│
└─ Body
   ├─ Logo FuelGuard
   ├─ Formulario login
   │  ├─ Email input
   │  ├─ Contraseña input
   │  ├─ Checkbox "Recordarme"
   │  └─ Botón "Ingresar"
   │
   ├─ Script localStorage
   │  └─ Verificación sesión
   │
   └─ Footer
      ├─ Links legales
      └─ Información
```

### Dashboard.html - Redirect Inteligente

```html
<script>
  const session = localStorage.getItem('fuelguard_session');
  window.location.replace(
    session ? 'Dashboar.html' : 'app.html'
  );
</script>
```

**Lógica:**
- Si existe sesión → Ir a Dashboar.html
- Si no existe → Ir a app.html

### Dashboar.html - Panel Principal

```html
├─ Navbar
│  ├─ Logo
│  ├─ Título página
│  └─ Botón logout
│
├─ Sidebar (mobile drawer)
│  ├─ Menú principal
│  ├─ Enlaces
│  └─ Perfil usuario
│
├─ Main Content
│  ├─ Widgets de datos
│  ├─ Gráficas
│  ├─ Mapas
│  └─ Alertas
│
└─ Footer
   └─ Info sistema
```

## 🔐 Autenticación

### Sistema localStorage

```javascript
// Login (en app.html)
function login() {
  // Validar credenciales
  localStorage.setItem('fuelguard_session', 'true');
  localStorage.setItem('fuelguard_user', 'usuario@mail.com');
  localStorage.setItem('fuelguard_token', 'JWT_TOKEN');
  
  // Redirigir
  window.location.href = 'Dashboar.html';
}

// Logout (en Dashboar.html)
function logout() {
  localStorage.removeItem('fuelguard_session');
  localStorage.removeItem('fuelguard_user');
  localStorage.removeItem('fuelguard_token');
  
  window.location.href = 'app.html';
}

// Verificar sesión
function checkSession() {
  if (!localStorage.getItem('fuelguard_session')) {
    window.location.href = 'app.html';
  }
}
```

## 📡 Integración MQTT

### WebSocket MQTT

```javascript
const client = new Paho.MQTT.Client(
  'broker.hivemq.com',
  8001,  // WebSocket port
  'clientId-' + Date.now()
);

// Conectar
client.connect({
  useSSL: true,
  onSuccess: onConnect,
  onFailure: onFailure
});

// Suscribirse
function onConnect() {
  client.subscribe('itics/mgti/isai/sensor');
  client.onMessageArrived = onMessageReceived;
}

// Recibir mensajes
function onMessageReceived(message) {
  const datos = JSON.parse(message.payloadString);
  actualizarDashboard(datos);
}
```

## 📊 Widgets y Visualización

### Widget de Flujo

```html
<div class="widget">
  <h3>Consumo Combustible</h3>
  <div class="metric">
    <span class="value" id="flujo-l-s">0.83</span>
    <span class="unit">L/s</span>
  </div>
  <div class="gauge">
    <canvas id="gauge-flujo"></canvas>
  </div>
</div>
```

### Widget de Ubicación

```html
<div class="widget map">
  <h3>Ubicación en Tiempo Real</h3>
  <div id="map" style="width: 100%; height: 300px;"></div>
  <p id="coords">Lat: 0.0 | Lon: 0.0</p>
</div>
```

### Widget de Alertas

```html
<div class="widget alerts">
  <h3>Alertas de Seguridad</h3>
  <div id="alerts-container">
    <!-- Alertas dinámicas -->
  </div>
</div>
```

## 🔄 Actualización en Tiempo Real

```javascript
// Actualizar cada 5 segundos
setInterval(actualizarDatos, 5000);

function actualizarDatos() {
  // Solicitar datos MQTT
  // Actualizar widgets
  // Mostrar animación
}
```

## 📱 Responsividad

### Breakpoints

```css
/* Mobile */
@media (max-width: 600px) {
  .container { width: 100%; }
  .sidebar { display: none; /* drawer */ }
  .widget { grid-column: span 1; }
}

/* Tablet */
@media (min-width: 601px) and (max-width: 1024px) {
  .container { width: 90%; }
  .widget { grid-column: span 2; }
}

/* Desktop */
@media (min-width: 1025px) {
  .container { width: 1180px; }
  .widget { grid-column: span auto; }
}
```

## 🎯 Funcionalidades

### Implementadas (v1.0)

- [x] Sistema de login/logout
- [x] Visualización de datos
- [x] Diseño responsivo
- [x] Tema oscuro
- [x] Iconos Font Awesome

### Planeadas (v1.1+)

- [ ] Gráficas históricas
- [ ] Exportar datos (CSV/PDF)
- [ ] Alertas push
- [ ] Modo offline
- [ ] Múltiples usuarios
- [ ] Permisos y roles
- [ ] API REST
- [ ] Aplicación móvil nativa

## 🔗 Integración con Grafana

### Embeds en HTML

```html
<iframe src="http://34.145.56.208:3000/d-solo/adghngx/monitoreo?orgId=1&panelId=2"
        width="450" height="200" frameborder="0">
</iframe>
```

**Paneles disponibles:**
- Panel 1: Mapa
- Panel 2: Flujo Real
- Panel 3: Estado Tapa
- Panel 4: Flujo Retorno

## 🆘 Troubleshooting

### "No puedo iniciar sesión"
- [ ] Borrar localStorage: F12 → Aplicación → Borrar
- [ ] Verificar credenciales
- [ ] Limpiar cookies
- [ ] Deshabilitar extensions

### "Dashboard no muestra datos"
- [ ] MQTT recibiendo? → Verificar con mosquitto_sub
- [ ] WebSocket abierto? → F12 → Network → WS
- [ ] Console tiene errores? → F12 → Console
- [ ] CORS habilitado?

### "Estilos no cargados"
- [ ] Google Fonts disponible?
- [ ] Font Awesome CDN accesible?
- [ ] Caché navegador limpiado? → Ctrl+Shift+Del
- [ ] CSS inline válido?

### "Mapa no se muestra"
- [ ] Leaflet librería cargada?
- [ ] OpenStreetMap disponible?
- [ ] Coordenadas válidas?
- [ ] Zoom correcto?

## 📦 Dependencias Externas

```html
<!-- Google Fonts -->
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800;900&display=swap" rel="stylesheet">

<!-- Font Awesome -->
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">

<!-- MQTT.js (opcional) -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/paho-mqtt/1.1.0/mqttws31.min.js"></script>

<!-- Leaflet Map (opcional) -->
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/leaflet.min.css">
<script src="https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.9.4/leaflet.min.js"></script>

<!-- Chart.js para gráficas -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js"></script>
```

## 🔒 Seguridad

### Consideraciones

- [ ] Tokens JWT con expiración
- [ ] HTTPS en producción
- [ ] CSP headers
- [ ] Input sanitization
- [ ] CORS restrictivo
- [ ] Rate limiting

### Implementación Mínima

```javascript
// Token con expiración
function guardarToken(token, expiresIn) {
  const expiry = Date.now() + (expiresIn * 1000);
  localStorage.setItem('fuelguard_token_expiry', expiry);
  localStorage.setItem('fuelguard_token', token);
}

// Validar token
function isTokenValid() {
  const expiry = localStorage.getItem('fuelguard_token_expiry');
  return Date.now() < expiry;
}
```

## 📊 Performance

### Optimizaciones

- Minificar CSS/JS
- Cargar imágenes lazy
- Cache de datos
- Compresión gzip
- CDN para assets

## 📚 Documentación Relacionada

- [README.md](../README.md) - Visión general
- [SHARE EMBED.txt](../SHARE%20EMBED.txt) - Embeds Grafana
- [NodeRed.json](../NodeRed.json) - Flujos procesamiento

## 🔗 Referencias

- [Leaflet Maps](https://leafletjs.com/)
- [Chart.js](https://www.chartjs.org/)
- [MQTT.js](https://github.com/mqttjs/MQTT.js)
- [MDN Web Docs](https://developer.mozilla.org/)

---

**Última actualización:** 16 de Mayo de 2026
