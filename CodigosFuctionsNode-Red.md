**fuction 8 GPS**

var datos = msg.payload;



// 1. Asegurar que los datos sean un objeto JSON

if (typeof datos === 'string') {

&#x20; try {

&#x20;   datos = JSON.parse(datos);

&#x20; } catch (e) {

&#x20;   node.error("Error parseando JSON de LilyGO");

&#x20; }

}



// 2. Extraer latitud y longitud (usando 'lng' como manda tu MQTT)

// Si no hay datos, se queda en la última posición conocida o base

let lat = (datos \&\& datos.lat !== undefined) ? Number(datos.lat) : 20.0283;

let lon = (datos \&\& datos.lng !== undefined) ? Number(datos.lng) : -99.2250;



// 3. Objeto para el Mapa (Marcador)

// Nota: Eliminamos el objeto "command" para que el mapa NO se mueva solo

msg.payload = {

&#x20; "name": "Piña",

&#x20; "lat": lat,

&#x20; "lon": lon,

&#x20; "icon": "truck",

&#x20; "iconColor": "#FFC107",

&#x20; "layer": "Unidades"

};



// 4. Texto con diseño para el Dashboard (Ubicación azul arriba)

// Usamos .toFixed(6) para captar la precisión real del GPS

msg.payload\_text = `

<div style="font-family:'Roboto Mono', monospace; color: rgb(68, 180, 255); font-size:14px;">

&#x20; <div style="display:flex; justify-content:space-between;">

&#x20;   <span>Latitud:</span>

&#x20;   <b>${lat.toFixed(6)}</b> 

&#x20; </div>

&#x20; <div style="display:flex; justify-content:space-between; margin-top:4px;">

&#x20;   <span>Longitud:</span>

&#x20;   <b>${lon.toFixed(6)}</b>

&#x20; </div>

</div>

`;



return msg;



**fuction 9 Estado de tapa**

var datos = msg.payload;

if (typeof datos === 'string') datos = JSON.parse(datos);



if (datos \&\& datos.alerta !== undefined) {

&#x20;   if (datos.alerta == 1) {

&#x20;       msg.payload = "CERRADA";

&#x20;       msg.background = "green"; // Cerrada = Seguro

&#x20;   } else {

&#x20;       msg.payload = "ABIERTA";

&#x20;       msg.background = "red";   // Abierta = Alerta!

&#x20;   }

} else {

&#x20;   msg.payload = "SIN DATO";

}

return msg;



**fuction 10 Flujo ida**

// Forzar que msg.payload sea el número del flujo 1

var datos = msg.payload;

if (typeof datos === 'string') datos = JSON.parse(datos);



if (datos \&\& datos.flujo1 !== undefined) {

&#x20;   msg.payload = Number(datos.flujo1); // <--- CAMBIO AQUÍ a flujo1

} else {

&#x20;   msg.payload = 0;

}

return msg;



**fuction 10-2 Flujo retorno**

// Forzar que msg.payload sea el número del flujo 2

var datos = msg.payload;

if (typeof datos === 'string') datos = JSON.parse(datos);



if (datos \&\& datos.flujo2 !== undefined) {

&#x20;   msg.payload = Number(datos.flujo2); // Extraemos flujo2

} else {

&#x20;   msg.payload = 0;

}

return msg;



**fuction 11 InfluxDB**

// 1. Capturar datos

var datos = msg.payload;



// Asegurar que los datos sean un objeto JSON

if (typeof datos === 'string') {

&#x20;   try {

&#x20;       datos = JSON.parse(datos);

&#x20;   } catch (e) {

&#x20;       node.error("Error parseando JSON");

&#x20;       return null;

&#x20;   }

}



// 2. EXTRAER COORDENADAS REALES

let latReal = (datos \&\& datos.lat !== undefined) ? Number(datos.lat) : null;

let lonReal = (datos \&\& datos.lng !== undefined) ? Number(datos.lng) : null;



// 3. EXTRAER OTROS DATOS 

let alertaReal = (datos \&\& datos.alerta !== undefined) ? Number(datos.alerta) : 0;

let flujo1Real = (datos \&\& datos.flujo1 !== undefined) ? Number(datos.flujo1) : 0;

let flujo2Real = (datos \&\& datos.flujo2 !== undefined) ? Number(datos.flujo2) : 0;



// =========================================================

// 🚀 NUEVO: CALCULAR EL CONSUMO (Diferencia de flujos)

// Usamos Math.max para asegurarnos de que el resultado nunca sea menor a 0

let consumoReal = Math.max(0, flujo1Real - flujo2Real);

// =========================================================



// 4. VALIDACIÓN (evita coordenadas 0,0)

if (latReal === 0 || lonReal === 0 || latReal === null || lonReal === null) {

&#x20;   node.warn("⚠️ Coordenadas invalidas, no se envia dato a InfluxDB");

&#x20;   return null;

}



// 5. ESTRUCTURA PARA INFLUXDB

msg.measurement = "monitoreo\_diesel\_tics";



msg.payload = \[

&#x20;   {

&#x20;       latitud: latReal,

&#x20;       longitud: lonReal,

&#x20;       estado\_tapa: alertaReal,

&#x20;       caudal\_flujo\_ida: flujo1Real,       

&#x20;       caudal\_flujo\_retorno: flujo2Real,

&#x20;       consumo\_actual: consumoReal         // <--- NUEVO CAMPO AGREGADO AQUÍ

&#x20;   },

&#x20;   {

&#x20;       unidad: "Camion\_Kiwi",

&#x20;       propietario: "Isai\_TICS",

&#x20;       sensor: "Hardware\_LilyGO\_Real"

&#x20;   }

];



return msg;

