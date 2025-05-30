# Laboratorio 2 – Parte B: Wi‑Fi AP + STA y Stack de Red 🌐

> **Objetivo:** Entender y configurar el ESP-NETIF y el Event Loop, luego implementar un Punto de Acceso (AP) y una Estación (STA) con el ESP32-S2.

---

## 📋 Descripción

1. **Interfaz de Red & Event Loop**

   * `esp_netif_init()`: inicializa el subsistema de red (ESP-NETIF), crea interfaces de red (Wi-Fi AP/STA, Ethernet, etc.).
   * `esp_event_loop_create_default()`: configura un loop de eventos global para manejar callbacks de red y Wi-Fi.
   * Ambas son **necesarias** antes de usar cualquier API de Wi-Fi o TCP/IP.

2. **Implementación del AP**

   * **Opciones de configuración**: SSID, canal, autenticación (WPA2, WPA3, abierto), máxima cantidad de estaciones, ocultar SSID, PMF.

   * **Pruebas**:

     * Conectar un móvil al SSID.
     * El monitor muestra logs de `WIFI_EVENT_AP_STACONNECTED` con MAC y AID.
     * Datos de IP no aplican (AP no asigna IP) al usar DHCP server opcional.
   * **Variaciones**:

     * **Abierto** (`WIFI_AUTH_OPEN`) → no pide contraseña.
     * **Oculto** (`ssid_hidden=1`) → el SSID no aparece en lista.
     * **WPA3** si está habilitado en menuconfig.




---

## 🚀 Instalación y uso

1. En `main/CMakeLists.txt`, asegúrate de incluir ambos componentes.
2. Ejecuta:

   ```bash
   idf.py set-target esp32s2
   idf.py menuconfig   # habilitar WPA3 si deseas
   idf.py build flash monitor
   ```
3. Observa en el monitor:

   * Logs de `esp_netif_init()` y `esp_event_loop_create_default()`.
   * Conexión STA con IP: `GOT_IP: 192.168.x.y`.
   * Conexión AP con logs de estaciones conectadas.

---

## ⚙️ Consideraciones y Comparación

* **Stack de red**:

  * `esp_netif_init()` es indispensable; sin esto no se crean las interfaces.
  * `esp_event_loop_create_default()` centraliza eventos (conexión, IP, desconexión).
* **AP vs STA**:

  * **AP**: tu ESP actúa como hotspot, no obtiene IP por DHCP, sino que podría servir DHCP si se configura.
  * **STA**: tu ESP se integra en una red existente, recibe IP y puede comunicarse con Internet.
* **Seguridad**:

  * WPA2-PSK vs WPA3-SAE vs abierto.
  * SSID oculto reduce descubrimiento casual.

---
