#include "mqtt_lib.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>
#include "cJSON.h"  
#define CJSON_CIRCULAR_LIMIT 32



static const char *TAG = "mqtt_lib";
static mqtt_message_callback_t user_callback = NULL;
static esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqtt_event_handler(void *args, esp_event_base_t base, int32_t event_id, void *event_data)//tiene como parametros (args,base,event_id,event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client; //accedo al cliente del evento

    switch ((esp_mqtt_event_id_t)event_id) {//mismo case que antes 
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected");
            esp_mqtt_client_subscribe(client, "/topic/qos1", 1); //en el caso de estar conectado se suscribe
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Mensaje recibido en topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Contenido: %.*s", event->data_len, event->data);

            if (user_callback) {
                // Copiar topic y data a buffers null-terminated
                char topic[event->topic_len + 1];//crea buffers
                char data[event->data_len + 1];
                memcpy(topic, event->topic, event->topic_len);//copia en los buffers
                topic[event->topic_len] = '\0';
                memcpy(data, event->data, event->data_len);
                data[event->data_len] = '\0';

                //recibe en formato JSON. 
                cJSON *root = cJSON_Parse(data);
                if (root) {
                    cJSON *cmd = cJSON_GetObjectItem(root, "comando");
                    if (cJSON_IsString(cmd)) {
                        user_callback(topic, cmd->valuestring, strlen(cmd->valuestring));
                    } else {
                        ESP_LOGW(TAG, "Campo 'comando' inválido o ausente");
                    }
                    cJSON_Delete(root);
                } else {
                    ESP_LOGW(TAG, "JSON inválido recibido");
                }
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT desconectado");
            break;

        default:
            break;
    }

}

esp_err_t mqtt_lib_init(const char *broker_uri, mqtt_message_callback_t callback)//funcion en si
{
    mqtt_client = client;
    user_callback = callback;

    if (broker_uri==NULL) {
        return ESP_ERR_INVALID_ARG;
    } //si el url del broker es null o el callback devuelve un error
    if(callback==NULL){
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,//revisar
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client==NULL){
      return ESP_FAIL;  
    } 

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    return esp_mqtt_client_start(client);
}
esp_err_t mqtt_lib_publish(const char *topic, const char *payload, int qos)
{
    if (mqtt_client==NULL){
        return ESP_FAIL;
    } 

    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, qos, 0);//id del msj publicado. si logra publicar un msj devuelve el id sino -1

    if (msg_id == -1) {
        ESP_LOGE(TAG, "Fallo al publicar en MQTT");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Mensaje publicado con ID %d", msg_id);
    return ESP_OK;
}
