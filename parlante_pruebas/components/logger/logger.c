#include "logger.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"
#include <stdio.h>
#include <inttypes.h>

static const char *TAG = "LOGGER";

// Variables globales simples
static bool g_logger_initialized = false;
static uint32_t g_event_counter = 0;

esp_err_t logger_init(void)
{
    if (g_logger_initialized) {
        ESP_LOGW(TAG, "Logger already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing logger...");

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_LOGW(TAG, "NVS partition needs to be erased");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
        return err;
    }

    // Open NVS
    ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle...");
    nvs_handle_t nvs_handle;
    err = nvs_open(LOGGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "NVS handle opened successfully");

    // Read event counter from NVS
    ESP_LOGI(TAG, "Reading event counter from NVS...");
    g_event_counter = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_u32(nvs_handle, LOGGER_NVS_KEY_COUNTER, &g_event_counter);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Event counter loaded successfully");
            ESP_LOGI(TAG, "Current event counter = %" PRIu32, g_event_counter);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG, "Event counter not initialized yet, starting from 0");
            g_event_counter = 0;
            break;
        default:
            ESP_LOGE(TAG, "Error (%s) reading event counter!", esp_err_to_name(err));
            nvs_close(nvs_handle);
            return err;
    }

    // Close NVS handle for now
    nvs_close(nvs_handle);

    g_logger_initialized = true;
    ESP_LOGI(TAG, "Logger initialized successfully");
    
    return ESP_OK;
}

esp_err_t logger_deinit(void)
{
    if (!g_logger_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing logger...");

    // Save current counter to NVS before deinitializing
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(LOGGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Saving final event counter to NVS...");
        err = nvs_set_u32(nvs_handle, LOGGER_NVS_KEY_COUNTER, g_event_counter);
        if (err == ESP_OK) {
            err = nvs_commit(nvs_handle);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Final event counter saved successfully");
            } else {
                ESP_LOGW(TAG, "Failed to commit final counter: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGW(TAG, "Failed to save final counter: %s", esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
    }

    g_logger_initialized = false;
    ESP_LOGI(TAG, "Logger deinitialized");
    
    return ESP_OK;
}

esp_err_t logger_log_event(logger_event_type_t event_type)
{
    if (!g_logger_initialized) {
        ESP_LOGE(TAG, "Logger not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (event_type < LOGGER_EVENT_PLAY || event_type > LOGGER_EVENT_STOP) {
        ESP_LOGE(TAG, "Invalid event type: %d", event_type);
        return ESP_ERR_INVALID_ARG;
    }

    // Increment counter
    g_event_counter++;

    ESP_LOGI(TAG, "Event logged: %s (total events: %" PRIu32 ")", 
             logger_event_type_to_string(event_type), g_event_counter);

    // Save to NVS immediately (simple approach)
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(LOGGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle for save!", esp_err_to_name(err));
        return err;
    }

    // Write updated counter to NVS
    ESP_LOGD(TAG, "Updating event counter in NVS...");
    err = nvs_set_u32(nvs_handle, LOGGER_NVS_KEY_COUNTER, g_event_counter);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save event counter!");
        nvs_close(nvs_handle);
        return err;
    }

    // Commit written value
    ESP_LOGD(TAG, "Committing updates in NVS...");
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes!");
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGD(TAG, "Event counter saved to NVS successfully");

    // Close NVS handle
    nvs_close(nvs_handle);

    return ESP_OK;
}

uint32_t logger_get_event_count(void)
{
    return g_event_counter;
}

const char* logger_event_type_to_string(logger_event_type_t event_type)
{
    switch (event_type) {
        case LOGGER_EVENT_PLAY:     return "PLAY";
        case LOGGER_EVENT_PAUSE:    return "PAUSE";
        case LOGGER_EVENT_NEXT:     return "NEXT";
        case LOGGER_EVENT_PREVIOUS: return "PREVIOUS";
        case LOGGER_EVENT_STOP:     return "STOP";
        default:                    return "UNKNOWN";
    }
}

void logger_print_info(void)
{
    if (!g_logger_initialized) {
        printf("Logger not initialized\n");
        return;
    }

    printf("\n=== LOGGER INFO ===\n");
    printf("Initialized: %s\n", g_logger_initialized ? "YES" : "NO");
    printf("Total events logged: %" PRIu32 "\n", g_event_counter);
    printf("NVS Namespace: %s\n", LOGGER_NVS_NAMESPACE);
    printf("NVS Key: %s\n", LOGGER_NVS_KEY_COUNTER);
    printf("===================\n\n");
}