#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>

// --- НАСТРОЙКИ ---
// const char* ssid = "KC-MED";
// const char* password = "g5+Nk7F4d";
const char* ssid = "Big_House";
const char* password = "Feya_Kit_2026";

// Замените на ваши прямые ссылки с GitHub
const char* version_url = "https://raw.githubusercontent.com/Magarich/esp32-test-oat/refs/heads/main/version.txt";
const char* firmware_url = "https://github.com/Magarich/esp32-test-oat/releases/latest/download/firmware.bin";

const String current_version = "0.0.5"; // Версия этой прошивки

long i = 0;

// --- ФУНКЦИИ ОБРАБОТКИ OTA ---
void update_started() { Serial.println("OTA: Начало загрузки..."); }
void update_finished() { Serial.println("OTA: Загрузка завершена!"); }
void update_progress(int cur, int total) { Serial.printf("OTA: Прогресс %d%%\n", (cur * 100) / total); }
void update_error(int err) { Serial.printf("OTA: Ошибка №%d\n", err); }

void run_ota_update() {
    WiFiClientSecure client;
    client.setInsecure(); // Для тестов (не проверяем SSL сертификат GitHub)

    // ВОТ ЭТА СТРОЧКА РЕШАЕТ ПРОБЛЕМУ 302 И ПЕРЕХОДОВ ПО РЕДИРЕКТАМ НА GITHUB
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    

    httpUpdate.onStart(update_started);
    httpUpdate.onEnd(update_finished);
    httpUpdate.onProgress(update_progress);
    httpUpdate.onError(update_error);

    Serial.println("OTA: Скачиваем новую прошивку...");
    t_httpUpdate_return ret = httpUpdate.update(client, firmware_url);

    if (ret == HTTP_UPDATE_FAILED) {
        Serial.printf("OTA: Ошибка (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    }
}

void check_for_updates() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    Serial.println("OTA: Проверка наличия новой версии...");
    http.begin(client, version_url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String new_version = http.getString();
        new_version.trim();

        if (new_version != current_version) {
            Serial.printf("OTA: Найдена версия %s (текущая %s). Обновляемся!\n", new_version.c_str(), current_version.c_str());
            run_ota_update();
        } else {
            Serial.println("OTA: Обновлений не требуется.");
        }
    } else {
        Serial.printf("OTA: Не удалось проверить версию, код ошибки: %d\n", httpCode);
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.printf("\n--- Устройство запущено. Версия: %s ---\n", current_version.c_str());

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi подключен!");
    Serial.println("IP адрес: " + WiFi.localIP().toString());

    // --- ПРОВЕРКА ROLLBACK ---
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // Если мы успешно дошли до сюда и есть интернет - подтверждаем прошивку
            Serial.println("OTA: Новая прошивка работает корректно. Подтверждаем.");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }

    // Проверяем обновление сразу при старте
    check_for_updates();
}

void loop() {
    // Индикация работы (мигаем раз в секунду)
    Serial.printf("%ld) ", i++);
    Serial.printf("Версия: %s", current_version.c_str());
    Serial.println();
    delay(1000);  
}