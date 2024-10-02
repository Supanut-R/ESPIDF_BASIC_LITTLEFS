#include <stdio.h>
#include <string.h>
#include "esp_littlefs.h" // new feature
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "cJSON.h"

#define TAG_LITTLEFS "LITTLEFS"
#define FILE_CONFIG_WIFI "/littlefs/wifi.json"

esp_err_t init_littlefs() {
    ESP_LOGI(TAG_LITTLEFS, "Initializing LittelFS");
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "littlefs",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG_LITTLEFS, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG_LITTLEFS, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG_LITTLEFS, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
    return ESP_OK;
}

esp_err_t open_file(const char* file_name, const char* mode, char *buffer, size_t length) {
    ESP_LOGI(TAG_LITTLEFS, "open file: %s", file_name);
    FILE *f = fopen(file_name, mode);
    if (f == NULL) {
        ESP_LOGE(TAG_LITTLEFS, "Failed to open file for reading: %s", file_name);
        return ESP_FAIL;
    }
    fgets(buffer, length, f);
    fclose(f);
    return ESP_OK;
}


void app_main() {
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_err_t ret = init_littlefs();
    ESP_ERROR_CHECK(ret); // ถ้ามัน error เราจะไม่ยอมให้มันไปทำอะไรทั้งสิ้น (มันไม่สามารถ mouth partition) มันจะติด boot loop ดีกว่าปล่อยให้มันทำไป
    if (ret == ESP_OK) {
        char *buffer = (char*)malloc(512);
        if (open_file(FILE_CONFIG_WIFI, "r",buffer, 512) == ESP_OK) { // r = read only
            ESP_LOGE(TAG_LITTLEFS, "Open success: %s", buffer);
            cJSON *root = cJSON_Parse(buffer);
            if (root == NULL) {
                ESP_LOGE(TAG_LITTLEFS, "This is NOT json format");
            } else {
                ESP_LOGI(TAG_LITTLEFS, "This is json format");
                if (cJSON_GetObjectItem(root, "ssid")) { // เช็คว่ามี key ที่ชื่อ ssid ไหม
                    char *ssid = cJSON_GetObjectItem(root,"ssid")->valuestring;
                    ESP_LOGI(TAG_LITTLEFS, "ssid=%s",ssid);
                }
                if (cJSON_GetObjectItem(root, "password")) {
                    char *pwd = cJSON_GetObjectItem(root,"password")-> valuestring; // ขึ้นอยู่กับประเภทของตัวแปร value___ ในที่นี้เก็บเป็น string
                    ESP_LOGI(TAG_LITTLEFS, "password=%s",pwd);
                }
                cJSON_Delete(root); // ทุกครั้งที่ ตัวแปร pointer มีการจอง memory ก็ต้องทำลายทิ้งด้วย
            }
        }
        free(buffer); // การคืน memory ในภาษา C
    }
}