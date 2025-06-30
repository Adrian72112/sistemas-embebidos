# 🎵 ESP32-S2 Kaluga Kit - Audio Player con Control MQTT

Un reproductor de audio completo y avanzado para la placa **ESP32-S2-Kaluga-1** que combina reproducción local de archivos PCM embebidos con **control remoto via MQTT**, implementando una arquitectura moderna basada en **colas de eventos asíncronos**.

## 🌟 Características Principales

- **🎧 Reproductor de Audio Completo** - Soporte para múltiples pistas PCM embebidas
- **🌐 Control Remoto MQTT** - Control total via comandos MQTT desde cualquier lugar
- **📱 WiFi Integrado** - Conexión automática a redes WiFi configuradas
- **🔄 Loop Continuo Inteligente** - Reproducción automática en bucle de pistas
- **⚡ Arquitectura Asíncrona** - Sistema de eventos no bloqueante y thread-safe
- **🎛️ Controles Completos** - Play, Pause, Next, Previous via MQTT
- **🛡️ Thread Safety Robusto** - Manejo seguro de concurrencia con FreeRTOS
- **📊 Logging Detallado** - Monitoreo completo de estados y eventos

## 🏗️ Arquitectura del Sistema

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         ESP32-S2 KALUGA KIT                                │
│                     Audio Player con Control MQTT                          │
├─────────────────────────────────────────────────────────────────────────────┤
│  [Internet] ←→ [WiFi] ←→ [MQTT Client] ←→ [Event Queue] ←→ [Audio Controller] │
│      ↓            ↓           ↓              ↓                ↓            │
│   Comandos    Conectividad  Broker      Async Events    Audio Hardware     │
│   Remotos     Inalámbrica   HiveMQ      Thread-Safe     ES8311 + I2S       │
└─────────────────────────────────────────────────────────────────────────────┘

FLUJO DE CONTROL:
1. Cliente MQTT envía comando (play/pause/next/previous)
2. ESP32 recibe comando via WiFi/MQTT
3. Callback MQTT procesa comando instantáneamente
4. Evento se envía a queue asíncrona (thread-safe)
5. Audio Event Task procesa evento en background
6. Hardware de audio ejecuta comando (ES8311 + I2S)
7. Pista se reproduce en loop continuo hasta nuevo comando
```

## 🎮 Control MQTT

### Comandos Disponibles

| Comando | Función | Descripción |
|---------|---------|-------------|
| `play` | ▶️ Reproducir | Inicia/reanuda reproducción en loop |
| `pause` | ⏸️ Pausar | Pausa la reproducción actual |
| `next` | ⏭️ Siguiente | Cambia a la siguiente pista |
| `previous` | ⏮️ Anterior | Cambia a la pista anterior |

### Broker MQTT
- **Broker:** `mqtt://broker.hivemq.com` (público)
- **Protocolo:** MQTT v3.1.1
- **QoS:** 0 (fire and forget)
- **Retain:** No
- **Autenticación:** No requerida

### Ejemplo de Comandos
```bash
# Usando mosquitto_pub
mosquitto_pub -h broker.hivemq.com -t "tu_topico_audio" -m "play"
mosquitto_pub -h broker.hivemq.com -t "tu_topico_audio" -m "pause"
mosquitto_pub -h broker.hivemq.com -t "tu_topico_audio" -m "next"
mosquitto_pub -h broker.hivemq.com -t "tu_topico_audio" -m "previous"

# Usando aplicación móvil MQTT
Topic: tu_topico_audio
Message: play
```

## 🎵 Playlist Integrada

El sistema incluye 6 pistas de audio embebidas:

1. **Victory** - Tema de victoria (victory8bit.pcm)
2. **8bit Classic** - Música retro clásica (8bit.pcm)
3. **Game Start** - Sonido de inicio de juego (start.pcm)
4. **Whistle** - Efecto de silbato (whistle.pcm)
5. **Lose** - Sonido de derrota (lose.pcm)
6. **Bueenass** - Audio personalizado (buenass.pcm)

### Características de Audio
- **Formato:** PCM sin compresión
- **Sample Rate:** 8000 Hz
- **Canales:** Mono
- **Resolución:** 16-bit
- **Codec:** ES8311 via I2S

## 🚀 Configuración y Uso

### 1. Configuración de Hardware

La ESP32-S2-Kaluga-1 incluye todo el hardware necesario:

#### Audio (ES8311 + I2S)
- **Codec:** ES8311 integrado
- **Amplificador:** TPA6403A
- **Conexión:** I2S + I2C
- **Salida:** Altavoz integrado + Jack 3.5mm

#### WiFi
- **Chip:** ESP32-S2 integrado
- **Antena:** PCB integrada
- **Protocolos:** 802.11 b/g/n

### 2. Compilación y Flash

```bash
# Clonar repositorio
git clone <tu-repo>
cd proyecto-final

# Configurar ESP-IDF
idf.py set-target esp32s2

# Configurar WiFi (importante!)
idf.py menuconfig
# → Example Connection Configuration
# → WiFi SSID: tu_red_wifi
# → WiFi Password: tu_password

# Compilar
idf.py build

# Flash al ESP32-S2
idf.py flash

# Monitor logs en tiempo real
idf.py monitor
```

### 3. Configuración WiFi

Antes de usar, configura tu red WiFi:

```bash
idf.py menuconfig
```

Navega a:
- `Example Connection Configuration`
- `WiFi SSID` → Tu red WiFi
- `WiFi Password` → Tu contraseña
- `WiFi Security Mode` → Seleccionar según tu red

### 4. Uso del Sistema

1. **Flash y Reinicia** el ESP32-S2
2. **Verifica conexión WiFi** en los logs
3. **Confirma conexión MQTT** (logs mostrarán broker conectado)
4. **Envía comandos MQTT** desde cualquier cliente
5. **Disfruta el control remoto** del audio

## 📊 Logs del Sistema

### Secuencia de Inicialización
```
I (123) main: 🔧 Inicializando sistema...
I (234) main: 📶 Conectando WiFi...
I (456) wifi: WiFi conectado con IP: 192.168.1.100
I (567) main: 🌐 Inicializando MQTT...
I (678) mqtt: Conectado a broker MQTT
I (789) main: 🎵 Inicializando audio controller...
I (890) audio_controller: 🎛️ Audio event task started
I (901) main: ✅ Sistema listo! Esperando comandos MQTT...
```

### Logs de Control MQTT
```
I (1234) main: 📨 Mensaje MQTT recibido -> Topic: audio, Data: play
I (1235) main: ▶️ Comando: PLAY - Enviando evento
I (1236) audio_controller: 🎵 Processing PLAY event
I (1237) audio_controller: ▶️ Playing: Victory
I (1238) audio_controller: 🎵 Playing: Victory (12845 bytes) - LOOP MODE
```

### Logs de Cambio de Pista
```
I (2345) main: 📨 Mensaje MQTT recibido -> Topic: audio, Data: next
I (2346) main: ⏭️ Comando: NEXT - Enviando evento
I (2347) audio_controller: ⏭️ Processing NEXT event
I (2348) audio_controller: ⏭️ Next: 8bit Classic (playing)
I (2349) audio_controller: 🎵 Playing: 8bit Classic (15234 bytes) - LOOP MODE
```

## 🔧 Componentes del Sistema

### Componentes Principales

1. **`audio_controller`** - Sistema de eventos de audio
   - Cola de eventos asíncrona
   - Manejo de hardware ES8311/I2S
   - Loop continuo de reproducción
   - Thread safety completo

2. **`mqtt_lib`** - Cliente MQTT
   - Conexión a broker público
   - Callback system
   - Reconexión automática

3. **`wifi_connection`** - Conectividad WiFi
   - Configuración automática
   - Manejo de eventos
   - Reconexión robusta

4. **`logger`** - Sistema de logging
   - Eventos de audio
   - Estados del sistema
   - Debug detallado

### Componentes Espressif
- **`espressif__es8311`** - Driver oficial del codec
- **ESP-IDF Core** - Framework base

## ⚙️ Configuración Avanzada

### Archivo `partitions.csv`
```csv
# Tabla de particiones personalizada para audio embebido
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x1F0000,
```

### Configuraciones ESP-IDF Importantes
```bash
# MQTT habilitado
CONFIG_MQTT_PROTOCOL_5=y

# Tabla de particiones personalizada
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"

# WiFi optimizado
CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32
```

## 🎯 Casos de Uso

### 1. Control desde Smartphone
- Instalar app MQTT (ej: MQTT Dashboard)
- Conectar al mismo WiFi
- Configurar broker: `broker.hivemq.com`
- Crear botones para cada comando

### 2. Integración Home Assistant
```yaml
# configuration.yaml
mqtt:
  broker: broker.hivemq.com

input_select:
  audio_control:
    name: "ESP32 Audio Control"
    options:
      - play
      - pause
      - next
      - previous

automation:
  - alias: "Audio Control"
    trigger:
      platform: state
      entity_id: input_select.audio_control
    action:
      service: mqtt.publish
      data:
        topic: "tu_topico_audio"
        payload: "{{ states('input_select.audio_control') }}"
```

### 3. Control desde Terminal/Script
```python
import paho.mqtt.publish as publish

def control_audio(command):
    publish.single("tu_topico_audio", command, hostname="broker.hivemq.com")

# Uso
control_audio("play")
control_audio("next")
```

## 🔍 Troubleshooting

### Problemas de WiFi
```
E (error) wifi: WiFi connection failed
```
**Solución:**
- Verificar SSID y password en `menuconfig`
- Comprobar que la red es 2.4GHz (no 5GHz)
- Verificar seguridad WPA2

### Problemas de MQTT
```
E (error) mqtt: Connection failed
```
**Solución:**
- Verificar conectividad WiFi primero
- Broker público puede estar sobrecargado (reintentar)
- Firewall puede bloquear puerto 1883

### Sin Audio
```
I (info) audio_controller: ▶️ Playing but no sound
```
**Solución:**
- Verificar volumen (configurar > 0 en código)
- Verificar conexión del altavoz
- Revisar configuración ES8311

## 📈 Rendimiento

### Especificaciones
- **Latencia MQTT:** < 100ms
- **Latencia de comando:** < 5ms (queue)
- **Memoria RAM:** ~32KB usado
- **Memoria Flash:** ~1.2MB (con audio embebido)
- **CPU Usage:** < 10% promedio

### Optimizaciones
- Callbacks MQTT no bloqueantes
- Buffers DMA para I2S
- Tareas con prioridades optimizadas
- Memory management eficiente

## 🛠️ Desarrollo y Extensión

### Agregar Nueva Pista
1. Convertir audio a PCM 8kHz mono
2. Agregar al `CMakeLists.txt`:
   ```cmake
   target_add_binary_data(proyecto-final.elf "audios/nueva_pista.pcm" BINARY)
   ```
3. Declarar en `main.c`:
   ```c
   extern const uint8_t nueva_pcm_start[] asm("_binary_nueva_pista_pcm_start");
   extern const uint8_t nueva_pcm_end[] asm("_binary_nueva_pista_pcm_end");
   ```
4. Agregar a playlist array

### Agregar Nuevo Comando MQTT
1. Agregar enum en `audio_controller.h`:
   ```c
   AUDIO_EVENT_TU_COMANDO,
   ```
2. Agregar case en event task:
   ```c
   case AUDIO_EVENT_TU_COMANDO:
       tu_funcion_internal();
       break;
   ```
3. Agregar en callback MQTT del main

## 📄 Licencia

Este proyecto está licenciado bajo **CC0-1.0** (Dominio Público). Libre para usar, modificar y distribuir sin restricciones.

## 🤝 Contribuciones

Las contribuciones son bienvenidas:
1. Fork el proyecto
2. Crear branch para feature (`git checkout -b feature/nueva-caracteristica`)
3. Commit cambios (`git commit -am 'Agregar nueva característica'`)
4. Push al branch (`git push origin feature/nueva-caracteristica`)
5. Crear Pull Request

---

## 📞 Soporte

Para soporte técnico o preguntas:
- Crear issue en GitHub
- Revisar logs con `idf.py monitor`
- Verificar configuración WiFi/MQTT

**¡Disfruta controlando tu música remotamente con ESP32-S2! 🎵📱**
