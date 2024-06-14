/* Advanced HTTPS OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_wifi.h"

TaskHandle_t task_handler = NULL;

static const char *TAG = "OTA";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

esp_app_desc_t running_app_info;

// Wifi
#define EXAMPLE_ESP_WIFI_SSID ""
#define EXAMPLE_ESP_WIFI_PASS ""
#define MAX_RETRY 10
static int retry_cnt = 0;
int WIFI_CONN = 0;

static esp_err_t wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        // Start station mode and connect to WiFi
        esp_wifi_connect();
        ESP_LOGI(TAG, "Trying to connect with Wi-Fi\n");
        break;

    case WIFI_EVENT_STA_CONNECTED:
        // WiFi connected
        WIFI_CONN = 1;
        ESP_LOGI(TAG, "Wi-Fi connected\n");
        break;

    case IP_EVENT_STA_GOT_IP:
        // Obtained IP address, start MQTT client
        ESP_LOGI(TAG, "got ip: starting HTTP Client\n");
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        // WiFi disconnected, retry connection if retries left
        ESP_LOGI(TAG, "disconnected: Retrying Wi-Fi\n");
        if (retry_cnt++ < MAX_RETRY)
        {
            esp_wifi_connect();
        }
        else
        {
            ESP_LOGI(TAG, "Max Retry Failed: Wi-Fi Connection\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        }
        break;

    default:
        break;
    }

    return ESP_OK;
}

void wifi_init(void)
{
    // Create default event loop
    esp_event_loop_create_default();

    // Register event handler for WiFi events
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);

    // Register event handler for IP events
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    // Initialize WiFi configuration
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,           // Set WiFi SSID
            .password = EXAMPLE_ESP_WIFI_PASS,       // Set WiFi password
            .threshold.authmode = WIFI_AUTH_WPA2_PSK // Set authentication mode
        },
    };

    // Initialize TCP/IP stack
    esp_netif_init();
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Set WiFi mode to station mode
    esp_wifi_set_mode(WIFI_MODE_STA);

    // Set WiFi configuration
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    // Start WiFi
    esp_wifi_start();
}

/* Event handler for catching system events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == ESP_HTTPS_OTA_EVENT)
    {
        switch (event_id)
        {
        case ESP_HTTPS_OTA_START:
            ESP_LOGI(TAG, "OTA started");
            break;
        case ESP_HTTPS_OTA_CONNECTED:
            ESP_LOGI(TAG, "Connected to server");
            break;
        case ESP_HTTPS_OTA_GET_IMG_DESC:
            ESP_LOGI(TAG, "Reading Image Description");
            break;
        case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
            ESP_LOGI(TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
            break;
        case ESP_HTTPS_OTA_DECRYPT_CB:
            ESP_LOGI(TAG, "Callback to decrypt function");
            break;
        case ESP_HTTPS_OTA_WRITE_FLASH:
            ESP_LOGD(TAG, "Writing to flash: %d written", *(int *)event_data);
            break;
        case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
            ESP_LOGI(TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
            break;
        case ESP_HTTPS_OTA_FINISH:
            ESP_LOGI(TAG, "OTA finish");
            break;
        case ESP_HTTPS_OTA_ABORT:
            ESP_LOGI(TAG, "OTA abort");
            break;
        }
    }
}

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        break;

    default:
        break;
    }
    return ESP_OK;
}

static void post_rest_fun(void)
{
    esp_http_client_config_t config_post = {
        .url = "http://192.168.56.178:5000/post-data",
        .method = HTTP_METHOD_POST,
        .cert_pem = NULL,
        .event_handler = client_event_post_handler};

    esp_http_client_handle_t client = esp_http_client_init(&config_post);
    char post_data[300];
    sprintf(post_data, "{\"project_name\":\"%s\", \"application_version\":\"%s\", \"secure_version\":\"%" PRIu32 "\", \"compile_time\":\"%s\", \"compile_date\":\"%s\"}", running_app_info.project_name, running_app_info.version, running_app_info.secure_version, running_app_info.time, running_app_info.date);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP POST Status = %d",
                 esp_http_client_get_status_code(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static esp_err_t get_app_info(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    return esp_ota_get_partition_description(running, &running_app_info);
}

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);

    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0)
    {
        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
        return ESP_FAIL;
    }

    return ESP_OK;
}
static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}

void OTA_task(void *pvParameter)
{
    while (1)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        if (WIFI_CONN)
        {
            ESP_LOGI(TAG, "Starting OTA ");

            esp_err_t ota_finish_err = ESP_OK;
            esp_http_client_config_t config = {
                .url = "http://192.168.56.178:5000/firmware",
                .method = HTTP_METHOD_GET,
                .cert_pem = (char *)server_cert_pem_start,
                .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
                .keep_alive_enable = true,
            };

            esp_https_ota_config_t ota_config = {
                .http_config = &config,
                .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
            };

            esp_https_ota_handle_t https_ota_handle = NULL;
            esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
            }

            esp_app_desc_t app_desc;
            err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_https_ota_get_img_desc failed");
                goto ota_end;
            }
            err = validate_image_header(&app_desc);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "image header verification failed");
                goto ota_end;
            }

            while (1)
            {
                err = esp_https_ota_perform(https_ota_handle);
                if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
                {
                    break;
                }
                // esp_https_ota_perform returns after every read operation which gives user the ability to
                // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
                // data read so far.
                ESP_LOGD(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
            }

            if (esp_https_ota_is_complete_data_received(https_ota_handle) != true)
            {
                // the OTA image was not completely received and user can customise the response to this situation.
                ESP_LOGE(TAG, "Complete data was not received.");
            }
            else
            {
                ota_finish_err = esp_https_ota_finish(https_ota_handle);
                if ((err == ESP_OK) && (ota_finish_err == ESP_OK))
                {
                    ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    esp_restart();
                }
                else
                {
                    if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED)
                    {
                        ESP_LOGE(TAG, "Image validation failed, image is corrupted");
                    }
                    ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
                }
            }

        ota_end:
            esp_https_ota_abort(https_ota_handle);
            ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "OTA app_main start");
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    // connect to the wifi network
    wifi_init();

    get_app_info();

    xTaskCreate(&OTA_task, "OTA task", 1024 * 8, NULL, 5, task_handler);
    while (1)
    {
        if (WIFI_CONN)
        {
            post_rest_fun();
            printf("Project name %s\n", running_app_info.project_name);
            printf("Application version  %s\n", running_app_info.version);
            printf("Secure version  %" PRIu32 "\n", running_app_info.secure_version);
            printf("Compile Time %s\n", running_app_info.time);
            printf("Compile Date %s\n", running_app_info.date);
            printf("------------------------------------\n");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}