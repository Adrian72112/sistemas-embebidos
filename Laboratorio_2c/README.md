# Laboratorio 2 – Parte C: Control de LED desde la Web 🚦

> **Objetivo:** Servir una web que permita apagar el LED al recargar la página y encenderlo en verde al enviar un formulario.

---

## 📋 Descripción

1. **GET /**: El handler `index_get_handler()` apaga el LED (RGB a 0,0,0) y sirve `index.html`.
2. **GET /style.css**: El handler `style_get_handler()` sirve `style.css` embebido.
3. **POST /enviar**: El handler `http_post_handler()` procesa el formulario (`data_input`), responde con "Dato recibido: X" y enciende el LED en **verde**.

---

## 🔧 Estructura de archivos

```
components/http_server/
├── CMakeLists.txt       # EMBED_FILES: index.html, style.css
├── http_server.h        # Interfaz del servidor HTTP
├── http_server.c        # Handlers GET(/), GET(/style.css), POST(/enviar), plus otros
├── index.html           # Página con formulario POST → /enviar
└── style.css            # Estilos centrados y responsivos

main/
└── main.c               # app_main(): inicializa LED, arranca servidor HTTP
```

---

## ⚙️ Requisitos

* ESP-IDF v5.x
* ESP32-S2 Kaluga-1
* Entorno de compilación ESP-IDF configurado

---

## 🚀 Instalación y uso

1. Clonar el repositorio.
2. Configurar el proyecto:

   ```bash
   idf.py set-target esp32s2
   idf.py menuconfig      # ajustar HTTP Server si es necesario
   ```
3. Compilar y flashear:

   ```bash
   idf.py build
   idf.py flash monitor
   ```
4. Abrir en el navegador la IP mostrada (ej. `http://192.168.4.1/`).

---

## 📖 Detalles de implementación

### main.c

```c
void app_main(void)
{
    // 1) Inicializa LED
    led_strip_t *strip = NULL;
    led_init(&strip);
    s_strip = strip;

    // 2) Pila TCP/IP, WDT y Wi-Fi
    esp_task_wdt_deinit();
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    example_connect();

    // 3) Arranca servidor HTTP
    httpd_handle_t server = start_webserver();

    // 4) Mantiene vivo
    while (server) {
        sleep(5);
    }
}
```

### http\_server.c

1. **GET /** → `index_get_handler()`:

   ```c
   led_set_color(s_strip, 0,0,0);  // apaga LED
   httpd_resp_send(req, index_html_start, index_html_len);
   ```
2. **GET /style.css** → `style_get_handler()`: sirve CSS embebido.
3. **POST /enviar** → `http_post_handler()`:

   * Valida `content_len` (0 < len ≤ POST\_BUF\_LEN).
   * Lee el cuerpo form-urlencoded y extrae `data_input`.
   * Si existe:

     * Loggea y responde `"Dato recibido: %s"`.
     * Enciende LED en verde: `led_set_color(s_strip, 0,255,0);`
   * Si falta, responde error sin cambiar LED.
4. **Registro de handlers** en `start_webserver()`:

   ```c
   httpd_register_uri_handler(server, &index_uri);
   httpd_register_uri_handler(server, &style_uri);
   httpd_register_uri_handler(server, &echo);   // POST /echo
   httpd_register_uri_handler(server, &ctrl);   // PUT /ctrl
   httpd_register_uri_handler(server, &any);    // ANY /any
   httpd_register_uri_handler(server, &(httpd_uri_t){
       .uri     = "/enviar",
       .method  = HTTP_POST,
       .handler = http_post_handler
   });
   ```

---

## 🔍 HTML & JS

```html
<!DOCTYPE html>
<html lang="en">
<head>...</head>
<body>
  <div class="container">
    <h1>ESP32-S2 Kaluga-1 TouchPad</h1>
    <form id="inputForm" method="post" action="/enviar">
      <label for="inputVal">Enter value:</label>
      <input id="inputVal" name="data_input" required />
      <button type="submit">Send</button>
    </form>
    <div id="result"></div>
  </div>
  <script>
    // Intercepta submit, envía POST x-www-form-urlencoded y actualiza #result
  </script>
</body>
</html>
```

---

## ⚙️ Consideraciones

* **Reload de /** apaga el LED.
* **POST /enviar** enciende LED verde.
* Usar `menuconfig → Component config → HTTP Server` para ampliar límites URI/headers si hace falta.
* Para multitarea real, reemplazar `sleep()` por `vTaskDelay()` y crear tareas.

---

## ✅ Conclusión

Integramos control físico (LED) con web:

* **GET /** → apaga LED
* **POST /enviar** → enciende LED verde y retorna la confirmación.

¡Parte C completada! 🎉
